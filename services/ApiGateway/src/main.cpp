#include <crow.h>
#include <crow/middlewares/cors.h>
#include "client/OrderClient.h"
#include "client/RestaurantClient.h"
#include "client/UserClient.h"
#include "client/PaymentClient.h"
#include "JwtManager.h"
#include "Database.h"
#include "DeliveryQuote.h"
#include "CorrelationMiddleware.h"
#include "RequestLoggingMiddleware.h"
#include <algorithm>
#include <cstdlib>
#include <chrono>
#include <cmath>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <sqlite3.h>
#include <ctime>

namespace
{
std::optional<int> authenticatedUserId(const crow::request& req)
{
    const std::string header = req.get_header_value("Authorization");
    const std::string prefix = "Bearer ";
    if (header.rfind(prefix, 0) != 0) return std::nullopt;
    const std::string token = header.substr(prefix.size());
    JwtManager jwt;
    if (!jwt.verifyToken(token)) return std::nullopt;
    try { return jwt.getUserId(token); }
    catch (const std::exception&) { return std::nullopt; }
}

crow::response unauthorized()
{
    crow::json::wvalue body;
    body["success"] = false;
    body["message"] = "A valid bearer token is required";
    return crow::response(401, body);
}

double distanceKm(double lat1, double lon1, double lat2, double lon2)
{
    constexpr double earthKm = 6371.0;
    constexpr double pi = 3.14159265358979323846;
    const auto radians = [pi](double value) { return value * pi / 180.0; };
    const double dLat = radians(lat2 - lat1);
    const double dLon = radians(lon2 - lon1);
    const double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
        std::cos(radians(lat1)) * std::cos(radians(lat2)) *
        std::sin(dLon / 2) * std::sin(dLon / 2);
    return earthKm * 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
}

crow::response jsonError(int status, const std::string& message)
{
    crow::json::wvalue body;
    body["success"] = false;
    body["message"] = message;
    return crow::response(status, body);
}

crow::response downstreamResponse(const HttpResult& result)
{
    const int status = gatewayStatusFor(result);
    const bool validHttpStatus = result.statusCode >= 100 && result.statusCode <= 599;
    if (!result.transportSucceeded() || !validHttpStatus)
    {
        CROW_LOG_ERROR << "Downstream request failed status=" << status
                       << " error=" << result.error;
        return jsonError(status, status == 504
            ? "A downstream service timed out"
            : "A downstream service is unavailable");
    }
    crow::response response(status, result.body);
    response.set_header("Content-Type", "application/json");
    return response;
}

bool validCoordinates(double lat, double lon) { return lat>=-90 && lat<=90 && lon>=-180 && lon<=180; }
bool envEnabled(const char* name) { const char* value=std::getenv(name); return value && std::string(value)=="1"; }
bool isLateNight()
{
    const std::time_t now=std::time(nullptr); std::tm local{};
#ifdef _WIN32
    localtime_s(&local,&now);
#else
    localtime_r(&now,&local);
#endif
    return local.tm_hour>=23 || local.tm_hour<6;
}
std::vector<DeliveryPoint> parsePolygon(const crow::json::rvalue& restaurant)
{
    std::vector<DeliveryPoint> result;
    if (!restaurant.has("deliveryPolygon") || std::string(restaurant["deliveryPolygon"].s()).empty()) return result;
    const auto json=crow::json::load(std::string(restaurant["deliveryPolygon"].s()));
    if (!json || json.t()!=crow::json::type::List) return result;
    for (const auto& point:json) if(point.t()==crow::json::type::List && point.size()>=2)
        result.push_back({point[0].d(),point[1].d()});
    return result;
}
DeliveryRules rulesFor(const crow::json::rvalue& restaurant)
{
    DeliveryRules rules;
    if(restaurant.has("deliveryRadiusKm")) rules.radiusKm=restaurant["deliveryRadiusKm"].d();
    if(restaurant.has("baseDeliveryFee")) rules.baseFee=restaurant["baseDeliveryFee"].d();
    if(restaurant.has("perKmFee")) rules.perKmFee=restaurant["perKmFee"].d();
    if(restaurant.has("preparationMinutes")) rules.preparationMinutes=restaurant["preparationMinutes"].i();
    rules.surge=envEnabled("DELIVERY_SURGE_MODE"); rules.rain=envEnabled("DELIVERY_RAIN_MODE"); rules.lateNight=isLateNight();
    return rules;
}
crow::json::wvalue quoteJson(const DeliveryQuote& quote, const DeliveryRules& rules)
{
    crow::json::wvalue out; out["serviceable"]=quote.serviceable; out["distanceKm"]=quote.distanceKm;
    out["deliveryFee"]=quote.fee; out["etaMinutes"]=quote.etaMinutes; out["zoneType"]=quote.zoneType;
    out["rules"]["surge"]=rules.surge; out["rules"]["rain"]=rules.rain; out["rules"]["lateNight"]=rules.lateNight;
    if (!quote.serviceable)
        out["message"] = "This restaurant does not deliver to the selected address (" +
            std::to_string(quote.distanceKm).substr(0, std::to_string(quote.distanceKm).find('.') + 2) +
            " km away; delivery zone " + std::to_string(rules.radiusKm).substr(0, std::to_string(rules.radiusKm).find('.') + 2) + " km)";
    return out;
}

crow::json::wvalue addressRow(sqlite3_stmt* statement)
{
    crow::json::wvalue row; row["id"]=sqlite3_column_int(statement,0); row["label"]=reinterpret_cast<const char*>(sqlite3_column_text(statement,1));
    row["recipient"]=reinterpret_cast<const char*>(sqlite3_column_text(statement,2)); row["phone"]=reinterpret_cast<const char*>(sqlite3_column_text(statement,3));
    row["addressLine"]=reinterpret_cast<const char*>(sqlite3_column_text(statement,4)); row["latitude"]=sqlite3_column_double(statement,5);
    row["longitude"]=sqlite3_column_double(statement,6); row["deliveryNotes"]=reinterpret_cast<const char*>(sqlite3_column_text(statement,7));
    row["isDefault"]=sqlite3_column_int(statement,8)==1; return row;
}

struct DriverLocation
{
    double latitude{}, longitude{}, accuracy{}, speed{}, heading{};
    std::string status, name, contact, vehicleType, vehiclePlate;
    long long updatedEpoch{}, statusChangedEpoch{};
};

int deliveryStatusIndex(const std::string& status)
{
    const std::vector<std::string> states={"ASSIGNED","PICKED_UP","ON_THE_WAY","ARRIVING","DELIVERED"};
    const auto found=std::find(states.begin(),states.end(),status);
    return found==states.end() ? -1 : static_cast<int>(std::distance(states.begin(),found));
}

int deliveryTransitionDelaySeconds()
{
    const char* configured=std::getenv("DELIVERY_STATUS_DELAY_SECONDS");
    if (!configured || !*configured) return 30;
    try { return std::clamp(std::stoi(configured),0,300); }
    catch (...) { return 30; }
}

long long currentEpoch()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool driverTokenValid(const crow::request& req)
{
    const char* configured = std::getenv("DRIVER_LOCATION_TOKEN");
    const std::string expected = configured && *configured ? configured : "local-driver-test-token";
    return req.get_header_value("X-Driver-Token") == expected;
}

bool saveDriverLocation(Database& database, int orderId, const DriverLocation& value)
{
    std::lock_guard<std::recursive_mutex> lock(database.mutex());
    const char* sql = "INSERT INTO driver_locations(order_id,latitude,longitude,accuracy_m,speed_mps,heading,delivery_status,driver_name,driver_contact,vehicle_type,vehicle_plate,updated_epoch,status_changed_epoch) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?) ON CONFLICT(order_id) DO UPDATE SET latitude=excluded.latitude,longitude=excluded.longitude,accuracy_m=excluded.accuracy_m,speed_mps=excluded.speed_mps,heading=excluded.heading,delivery_status=excluded.delivery_status,driver_name=excluded.driver_name,driver_contact=excluded.driver_contact,vehicle_type=excluded.vehicle_type,vehicle_plate=excluded.vehicle_plate,updated_epoch=excluded.updated_epoch,status_changed_epoch=CASE WHEN driver_locations.delivery_status=excluded.delivery_status THEN driver_locations.status_changed_epoch ELSE excluded.status_changed_epoch END;";
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database.connection(), sql, -1, &statement, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(statement, 1, orderId); sqlite3_bind_double(statement, 2, value.latitude);
    sqlite3_bind_double(statement, 3, value.longitude); sqlite3_bind_double(statement, 4, value.accuracy);
    sqlite3_bind_double(statement, 5, value.speed); sqlite3_bind_double(statement, 6, value.heading);
    sqlite3_bind_text(statement, 7, value.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 8, value.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 9, value.contact.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 10, value.vehicleType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 11, value.vehiclePlate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(statement, 12, value.updatedEpoch);
    sqlite3_bind_int64(statement, 13, value.statusChangedEpoch);
    const int result = sqlite3_step(statement); sqlite3_finalize(statement);
    return result == SQLITE_DONE;
}

std::optional<DriverLocation> loadDriverLocation(Database& database, int orderId)
{
    std::lock_guard<std::recursive_mutex> lock(database.mutex());
    sqlite3_stmt* statement = nullptr;
    const char* sql = "SELECT latitude,longitude,accuracy_m,speed_mps,heading,delivery_status,driver_name,driver_contact,vehicle_type,vehicle_plate,updated_epoch,status_changed_epoch FROM driver_locations WHERE order_id=?;";
    if (sqlite3_prepare_v2(database.connection(), sql, -1, &statement, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_int(statement, 1, orderId); std::optional<DriverLocation> result;
    if (sqlite3_step(statement) == SQLITE_ROW)
    {
        DriverLocation value; value.latitude = sqlite3_column_double(statement, 0); value.longitude = sqlite3_column_double(statement, 1);
        value.accuracy = sqlite3_column_double(statement, 2); value.speed = sqlite3_column_double(statement, 3); value.heading = sqlite3_column_double(statement, 4);
        value.status = reinterpret_cast<const char*>(sqlite3_column_text(statement, 5)); value.name = reinterpret_cast<const char*>(sqlite3_column_text(statement, 6));
        value.contact = reinterpret_cast<const char*>(sqlite3_column_text(statement, 7)); value.vehicleType = reinterpret_cast<const char*>(sqlite3_column_text(statement, 8));
        value.vehiclePlate = reinterpret_cast<const char*>(sqlite3_column_text(statement, 9)); value.updatedEpoch = sqlite3_column_int64(statement, 10);
        value.statusChangedEpoch = sqlite3_column_int64(statement, 11); result = value;
    }
    sqlite3_finalize(statement); return result;
}

void removeDriverLocation(Database& database, int orderId)
{
    std::lock_guard<std::recursive_mutex> lock(database.mutex());
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database.connection(), "DELETE FROM driver_locations WHERE order_id=?;", -1, &statement, nullptr) != SQLITE_OK) return;
    sqlite3_bind_int(statement, 1, orderId); sqlite3_step(statement); sqlite3_finalize(statement);
}
}

int main()
{
    crow::App<crow::CORSHandler, CorrelationMiddleware, RequestLoggingMiddleware> app;
    RequestLoggingMiddleware::setServiceName("gateway");
    Database deliveryDatabase("delivery.db");
    deliveryDatabase.createDriverLocationTable();
    deliveryDatabase.createCustomerAddressTable();

    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .origin("*")
        .headers("Content-Type", "Authorization", "Idempotency-Key", "X-Webhook-Secret", "X-Driver-Token", "X-Correlation-ID")
        .expose("X-Correlation-ID")
        .methods(
            crow::HTTPMethod::GET,
            crow::HTTPMethod::POST,
            crow::HTTPMethod::PUT,
            crow::HTTPMethod::DELETE,
            crow::HTTPMethod::OPTIONS);

RestaurantClient restaurantClient;
UserClient userClient;
PaymentClient paymentClient;


CROW_ROUTE(app, "/register")
.methods(crow::HTTPMethod::POST)
([&userClient](const crow::request& req)
{
    return downstreamResponse(userClient.registerUser(req.body));
});

CROW_ROUTE(app, "/login")
.methods(crow::HTTPMethod::POST)
([&userClient](const crow::request& req)
{
    return downstreamResponse(userClient.login(req.body));
});

CROW_ROUTE(app, "/me")
.methods(crow::HTTPMethod::GET)
([&userClient](const crow::request& req)
{
    const auto userId = authenticatedUserId(req);
    if (!userId) return unauthorized();
    return downstreamResponse(userClient.getUserById(
        *userId, req.get_header_value("Authorization")));
});

CROW_ROUTE(app, "/users")
([&userClient](const crow::request& req)
{
    switch (req.method)
    {
        case crow::HTTPMethod::GET:
            return downstreamResponse(userClient.getAllUsers(
                req.get_header_value("Authorization")));

        default:
            return crow::response(405);
    }
});

CROW_ROUTE(app, "/users/<int>")
([&userClient](const crow::request& req, int id)
{
    switch (req.method)
    {
        case crow::HTTPMethod::GET:
            return downstreamResponse(userClient.getUserById(
                id, req.get_header_value("Authorization")));

        case crow::HTTPMethod::PUT:
            return downstreamResponse(userClient.updateUser(
                id, req.body, req.get_header_value("Authorization")));

        case crow::HTTPMethod::DELETE:
            return downstreamResponse(userClient.deleteUser(
                id, req.get_header_value("Authorization")));

        default:
            return crow::response(405);
    }
});
CROW_ROUTE(app, "/restaurants")
.methods(crow::HTTPMethod::GET, crow::HTTPMethod::POST)
([&restaurantClient](const crow::request& req)
{
    switch (req.method)
    {
        case crow::HTTPMethod::GET:
            return downstreamResponse(restaurantClient.getAllRestaurants());

        case crow::HTTPMethod::POST:
            return downstreamResponse(restaurantClient.registerRestaurant(req.body));

        default:
            return crow::response(405);
    }
});

CROW_ROUTE(app, "/restaurants/<int>")
.methods(
    crow::HTTPMethod::GET,
    crow::HTTPMethod::PUT,
    crow::HTTPMethod::DELETE)
([&restaurantClient](const crow::request& req, int id)
{
    switch (req.method)
    {
        case crow::HTTPMethod::GET:
            return downstreamResponse(restaurantClient.getRestaurantById(id));

        case crow::HTTPMethod::PUT:
            return downstreamResponse(restaurantClient.updateRestaurant(id, req.body));

        case crow::HTTPMethod::DELETE:
            return downstreamResponse(restaurantClient.deleteRestaurant(id));

        default:
            return crow::response(405);
    }
});
    CROW_ROUTE(app, "/health")
    ([]()
    {
        return "API Gateway is Healthy!";
    });

    OrderClient client;

    CROW_ROUTE(app, "/addresses")
    .methods(crow::HTTPMethod::GET, crow::HTTPMethod::POST)
    ([&deliveryDatabase](const crow::request& req)
    {
        const auto userId=authenticatedUserId(req); if(!userId) return unauthorized();
        std::lock_guard<std::recursive_mutex> lock(deliveryDatabase.mutex());
        if(req.method==crow::HTTPMethod::GET) {
            sqlite3_stmt* statement=nullptr; crow::json::wvalue rows; std::size_t index=0;
            const char* sql="SELECT id,label,recipient,phone,address_line,latitude,longitude,delivery_notes,is_default FROM customer_addresses WHERE user_id=? ORDER BY is_default DESC,id DESC;";
            if(sqlite3_prepare_v2(deliveryDatabase.connection(),sql,-1,&statement,nullptr)!=SQLITE_OK) return jsonError(500,"Could not load addresses");
            sqlite3_bind_int(statement,1,*userId); while(sqlite3_step(statement)==SQLITE_ROW) rows[index++]=addressRow(statement); sqlite3_finalize(statement);
            return crow::response(rows);
        }
        const auto input=crow::json::load(req.body);
        if(!input || !input.has("label") || !input.has("recipient") || !input.has("phone") || !input.has("addressLine") || !input.has("latitude") || !input.has("longitude"))
            return jsonError(400,"Label, recipient, phone, address and coordinates are required");
        const double lat=input["latitude"].d(), lon=input["longitude"].d();
        if(!validCoordinates(lat,lon) || std::string(input["phone"].s()).size()<7) return jsonError(422,"Address coordinates or phone are invalid");
        const bool makeDefault=!input.has("isDefault") || input["isDefault"].b();
        if(makeDefault) { sqlite3_stmt* clear=nullptr; sqlite3_prepare_v2(deliveryDatabase.connection(),"UPDATE customer_addresses SET is_default=0 WHERE user_id=?;",-1,&clear,nullptr); sqlite3_bind_int(clear,1,*userId); sqlite3_step(clear); sqlite3_finalize(clear); }
        sqlite3_stmt* statement=nullptr; const char* sql="INSERT INTO customer_addresses(user_id,label,recipient,phone,address_line,latitude,longitude,delivery_notes,is_default,created_epoch) VALUES(?,?,?,?,?,?,?,?,?,?);";
        if(sqlite3_prepare_v2(deliveryDatabase.connection(),sql,-1,&statement,nullptr)!=SQLITE_OK) return jsonError(500,"Could not save address");
        const std::string label=input["label"].s(), recipient=input["recipient"].s();
        const std::string phone=input["phone"].s(), addressLine=input["addressLine"].s();
        sqlite3_bind_int(statement,1,*userId); sqlite3_bind_text(statement,2,label.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_text(statement,3,recipient.c_str(),-1,SQLITE_TRANSIENT);
        sqlite3_bind_text(statement,4,phone.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_text(statement,5,addressLine.c_str(),-1,SQLITE_TRANSIENT);
        sqlite3_bind_double(statement,6,lat); sqlite3_bind_double(statement,7,lon); const std::string notes=input.has("deliveryNotes")?std::string(input["deliveryNotes"].s()):"";
        sqlite3_bind_text(statement,8,notes.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_int(statement,9,makeDefault?1:0); sqlite3_bind_int64(statement,10,currentEpoch());
        const int result=sqlite3_step(statement); sqlite3_finalize(statement); if(result!=SQLITE_DONE) return jsonError(500,"Could not save address");
        crow::json::wvalue out; out["success"]=true; out["id"]=static_cast<int>(sqlite3_last_insert_rowid(deliveryDatabase.connection())); return crow::response(201,out);
    });

    CROW_ROUTE(app, "/addresses/<int>/select")
    .methods(crow::HTTPMethod::PUT)
    ([&deliveryDatabase](const crow::request& req,int id)
    {
        const auto userId=authenticatedUserId(req); if(!userId) return unauthorized(); std::lock_guard<std::recursive_mutex> lock(deliveryDatabase.mutex());
        sqlite3_stmt* statement=nullptr; sqlite3_prepare_v2(deliveryDatabase.connection(),"UPDATE customer_addresses SET is_default=1 WHERE id=? AND user_id=?;",-1,&statement,nullptr);
        sqlite3_bind_int(statement,1,id); sqlite3_bind_int(statement,2,*userId); sqlite3_step(statement); const bool found=sqlite3_changes(deliveryDatabase.connection())>0; sqlite3_finalize(statement);
        if(!found) return jsonError(404,"Address not found"); sqlite3_prepare_v2(deliveryDatabase.connection(),"UPDATE customer_addresses SET is_default=0 WHERE user_id=? AND id<>?;",-1,&statement,nullptr);
        sqlite3_bind_int(statement,1,*userId); sqlite3_bind_int(statement,2,id); sqlite3_step(statement); sqlite3_finalize(statement);
        crow::json::wvalue out; out["success"]=true; return crow::response(out);
    });

    CROW_ROUTE(app, "/addresses/<int>")
    .methods(crow::HTTPMethod::DELETE)
    ([&deliveryDatabase](const crow::request& req,int id)
    {
        const auto userId=authenticatedUserId(req); if(!userId) return unauthorized(); std::lock_guard<std::recursive_mutex> lock(deliveryDatabase.mutex());
        sqlite3_stmt* statement=nullptr; sqlite3_prepare_v2(deliveryDatabase.connection(),"DELETE FROM customer_addresses WHERE id=? AND user_id=?;",-1,&statement,nullptr);
        sqlite3_bind_int(statement,1,id); sqlite3_bind_int(statement,2,*userId); sqlite3_step(statement); const bool removed=sqlite3_changes(deliveryDatabase.connection())>0; sqlite3_finalize(statement);
        if(!removed) return jsonError(404,"Address not found"); crow::json::wvalue out; out["success"]=true; return crow::response(out);
    });

    CROW_ROUTE(app, "/delivery/quote")
    .methods(crow::HTTPMethod::POST)
    ([&restaurantClient](const crow::request& req)
    {
        const auto input=crow::json::load(req.body);
        if(!input || !input.has("restaurantId") || !input.has("latitude") || !input.has("longitude")) return jsonError(400,"Restaurant and coordinates are required");
        const double lat=input["latitude"].d(),lon=input["longitude"].d(); if(!validCoordinates(lat,lon)) return jsonError(422,"Coordinates are invalid");
        const auto restaurantResult=restaurantClient.getRestaurantById(input["restaurantId"].i());
        if (!restaurantResult.transportSucceeded() || restaurantResult.statusCode != 200)
            return downstreamResponse(restaurantResult);
        const auto restaurant=crow::json::load(restaurantResult.body);
        if(!restaurant || !restaurant.has("latitude") || !restaurant.has("longitude")) return jsonError(404,"Restaurant location is unavailable");
        const auto rules=rulesFor(restaurant); const auto quote=calculateDeliveryQuote({restaurant["latitude"].d(),restaurant["longitude"].d()},{lat,lon},rules,parsePolygon(restaurant));
        return crow::response(quote.serviceable?200:422,quoteJson(quote,rules));
    });

    CROW_ROUTE(app, "/orders")
        .methods(crow::HTTPMethod::POST)
    ([&client, &restaurantClient, &deliveryDatabase](const crow::request& req)
    {
        const auto userId = authenticatedUserId(req);
        if (!userId) return unauthorized();
        const auto input = crow::json::load(req.body);
        if (!input || !input.has("restaurantId") || !input.has("totalAmount") ||
            !input.has("deliveryLatitude") || !input.has("deliveryLongitude") || !input.has("deliveryAddress"))
            return jsonError(400, "Restaurant, amount, and delivery location are required");
        double deliveryLat = input["deliveryLatitude"].d();
        double deliveryLon = input["deliveryLongitude"].d();
        std::string deliveryAddress=input["deliveryAddress"].s();
        if(input.has("addressId")) {
            std::lock_guard<std::recursive_mutex> lock(deliveryDatabase.mutex()); sqlite3_stmt* statement=nullptr;
            sqlite3_prepare_v2(deliveryDatabase.connection(),"SELECT address_line,latitude,longitude FROM customer_addresses WHERE id=? AND user_id=?;",-1,&statement,nullptr);
            sqlite3_bind_int(statement,1,input["addressId"].i()); sqlite3_bind_int(statement,2,*userId);
            if(sqlite3_step(statement)!=SQLITE_ROW){ sqlite3_finalize(statement); return jsonError(422,"Select one of your saved delivery addresses"); }
            deliveryAddress=reinterpret_cast<const char*>(sqlite3_column_text(statement,0)); deliveryLat=sqlite3_column_double(statement,1); deliveryLon=sqlite3_column_double(statement,2); sqlite3_finalize(statement);
        }
        if (deliveryLat < -90 || deliveryLat > 90 || deliveryLon < -180 || deliveryLon > 180)
            return jsonError(422, "Delivery coordinates are invalid");
        const auto restaurantResult = restaurantClient.getRestaurantById(input["restaurantId"].i());
        if (!restaurantResult.transportSucceeded() || restaurantResult.statusCode != 200)
            return downstreamResponse(restaurantResult);
        const auto restaurant = crow::json::load(restaurantResult.body);
        if (!restaurant || !restaurant.has("latitude") || !restaurant.has("longitude"))
            return jsonError(404, "Restaurant location is unavailable");
        const auto rules=rulesFor(restaurant); const auto quote=calculateDeliveryQuote({restaurant["latitude"].d(),restaurant["longitude"].d()},{deliveryLat,deliveryLon},rules,parsePolygon(restaurant));
        if (!quote.serviceable)
            return jsonError(422, "This address is outside the restaurant delivery area");
        crow::json::wvalue body;
        body["userId"] = *userId;
        body["restaurantId"] = input["restaurantId"].i();
        const double subtotal=input.has("subtotal")?input["subtotal"].d():input["totalAmount"].d();
        const double discount=input.has("discountAmount")?input["discountAmount"].d():0.0;
        body["totalAmount"] = subtotal-discount+quote.fee;
        body["deliveryLatitude"] = deliveryLat;
        body["deliveryLongitude"] = deliveryLon;
        body["deliveryAddress"] = deliveryAddress;
        body["itemSummary"] = input.has("itemSummary") ? std::string(input["itemSummary"].s()) : "";
        body["subtotal"] = subtotal;
        body["discountAmount"] = discount;
        body["deliveryFee"] = quote.fee;
        return downstreamResponse(client.createOrder(body.dump()));
    });

CROW_ROUTE(app, "/orders")
.methods(crow::HTTPMethod::GET)
([&client](const crow::request& req)
{
    const auto userId = authenticatedUserId(req);
    if (!userId) return unauthorized();
    const auto orderResult = client.getAllOrders();
    if (!orderResult.transportSucceeded() || orderResult.statusCode != 200)
        return downstreamResponse(orderResult);
    const auto orders = crow::json::load(orderResult.body);
    if (!orders) return jsonError(502, "Order Service returned an invalid response");
    crow::json::wvalue filtered;
    std::size_t index = 0;
    for (const auto& order : orders)
    {
        if (order.has("userId") && order["userId"].i() == *userId)
            filtered[index++] = order;
    }
    return crow::response(filtered);
});

CROW_ROUTE(app, "/orders/<int>")
.methods(crow::HTTPMethod::GET)
([&client](int id)
{
    return downstreamResponse(client.getOrderById(id));
});

CROW_ROUTE(app, "/restaurants/discover")
.methods(crow::HTTPMethod::GET)
([&restaurantClient](const crow::request& req)
{
    const char* latParam = req.url_params.get("lat");
    const char* lonParam = req.url_params.get("lon");
    if (!latParam || !lonParam) return jsonError(400, "lat and lon are required");
    double latitude = 0.0;
    double longitude = 0.0;
    try { latitude = std::stod(latParam); longitude = std::stod(lonParam); }
    catch (const std::exception&) { return jsonError(422, "Coordinates are invalid"); }
    if (latitude < -90 || latitude > 90 || longitude < -180 || longitude > 180)
        return jsonError(422, "Coordinates are invalid");

    const auto providerResult = restaurantClient.discoverNearby(latitude, longitude);
    if (!providerResult.transportSucceeded() || providerResult.statusCode != 200)
        return downstreamResponse(providerResult);
    const auto provider = crow::json::load(providerResult.body);
    if (!provider || !provider.has("elements"))
        return jsonError(503, "Nearby restaurant provider is temporarily unavailable");

    std::unordered_set<std::string> existingNames;
    const auto existingResult = restaurantClient.getAllRestaurants();
    if (!existingResult.transportSucceeded() || existingResult.statusCode != 200)
        return downstreamResponse(existingResult);
    const auto existing = crow::json::load(existingResult.body);
    if (existing)
        for (const auto& restaurant : existing)
            if (restaurant.has("name")) existingNames.insert(std::string(restaurant["name"].s()));

    int discovered = 0;
    int imported = 0;
    for (const auto& element : provider["elements"])
    {
        if (discovered >= 20 || !element.has("tags") || !element["tags"].has("name")) continue;
        const auto tags = element["tags"];
        ++discovered;
        const std::string name = std::string(tags["name"].s());
        if (existingNames.count(name)) continue;
        double restaurantLat = element.has("lat") ? element["lat"].d() :
            (element.has("center") && element["center"].has("lat") ? element["center"]["lat"].d() : latitude);
        double restaurantLon = element.has("lon") ? element["lon"].d() :
            (element.has("center") && element["center"].has("lon") ? element["center"]["lon"].d() : longitude);
        std::string address = tags.has("addr:full") ? std::string(tags["addr:full"].s()) :
            (tags.has("addr:street") ? std::string(tags["addr:street"].s()) : "OpenStreetMap nearby listing");
        std::string phone = tags.has("phone") ? std::string(tags["phone"].s()) : "Not listed";
        std::string imageUrl;
        if (tags.has("image"))
        {
            const std::string candidate = std::string(tags["image"].s());
            if (candidate.rfind("https://", 0) == 0 || candidate.rfind("http://", 0) == 0)
                imageUrl = candidate;
        }
        else if (tags.has("wikimedia_commons"))
        {
            std::string file = std::string(tags["wikimedia_commons"].s());
            if (file.rfind("File:", 0) == 0) file = file.substr(5);
            if (!file.empty()) imageUrl = "https://commons.wikimedia.org/wiki/Special:Redirect/file/" + file;
        }
        crow::json::wvalue body;
        body["name"] = name;
        body["address"] = address;
        body["phone"] = phone;
        body["rating"] = 4.3;
        body["latitude"] = restaurantLat;
        body["longitude"] = restaurantLon;
        body["deliveryRadiusKm"] = 8.0;
        body["imageUrl"] = imageUrl;
        const auto importedResult = restaurantClient.registerRestaurant(body.dump());
        if (!importedResult.transportSucceeded() || importedResult.statusCode < 200 || importedResult.statusCode >= 300)
            continue;
        existingNames.insert(name);
        ++imported;
    }

    std::string city = "Current area";
    if (latitude >= 12.7 && latitude <= 13.3 && longitude >= 77.3 && longitude <= 77.9)
        city = "Bengaluru";
    else if (latitude >= 22.8 && latitude <= 23.3 && longitude >= 72.3 && longitude <= 72.8)
        city = "Ahmedabad";
    crow::json::wvalue response;
    response["city"] = city;
    response["discovered"] = discovered;
    response["imported"] = imported;
    response["provider"] = "OpenStreetMap Overpass";
    response["attribution"] = "© OpenStreetMap contributors";
    return crow::response(response);
});

CROW_ROUTE(app, "/driver/orders/<int>/location")
.methods(crow::HTTPMethod::POST)
([&client, &paymentClient, &deliveryDatabase](const crow::request& req, int id)
{
    if (!driverTokenValid(req)) return jsonError(401, "A valid driver location token is required");
    const auto input = crow::json::load(req.body);
    if (!input || !input.has("latitude") || !input.has("longitude"))
        return jsonError(400, "latitude and longitude are required");
    const double latitude = input["latitude"].d(), longitude = input["longitude"].d();
    if (!std::isfinite(latitude) || !std::isfinite(longitude) || latitude < -90 || latitude > 90 || longitude < -180 || longitude > 180)
        return jsonError(422, "Driver coordinates are invalid");
    const auto orderResult = client.getOrderById(id);
    if (!orderResult.transportSucceeded() || orderResult.statusCode != 200)
        return downstreamResponse(orderResult);
    const auto order = crow::json::load(orderResult.body);
    if (!order || !order.has("id")) return jsonError(404, "Order not found");
    const auto paymentResult = paymentClient.getPaymentForOrder(id);
    if (!paymentResult.transportSucceeded() || paymentResult.statusCode != 200)
        return downstreamResponse(paymentResult);
    const auto payment = crow::json::load(paymentResult.body);
    if (!payment || !payment.has("status") || std::string(payment["status"].s()) != "succeeded")
        return jsonError(409, "Driver location is accepted only after verified payment");
    DriverLocation location;
    location.latitude = latitude; location.longitude = longitude;
    location.accuracy = input.has("accuracy") ? input["accuracy"].d() : 0.0;
    location.speed = input.has("speed") ? std::max(0.0, input["speed"].d()) : 0.0;
    location.heading = input.has("heading") ? input["heading"].d() : 0.0;
    location.status = input.has("status") ? std::string(input["status"].s()) : "ASSIGNED";
    const std::unordered_set<std::string> allowed = {"ASSIGNED", "PICKED_UP", "ON_THE_WAY", "ARRIVING", "DELIVERED"};
    if (!allowed.count(location.status)) return jsonError(422, "Invalid delivery status");
    location.name = input.has("driverName") ? std::string(input["driverName"].s()) : "Delivery partner";
    location.contact = input.has("driverContact") ? std::string(input["driverContact"].s()) : "Not shared";
    location.vehicleType = input.has("vehicleType") ? std::string(input["vehicleType"].s()) : "Delivery vehicle";
    location.vehiclePlate = input.has("vehiclePlate") ? std::string(input["vehiclePlate"].s()) : "Not shared";
    location.updatedEpoch = currentEpoch();
    const auto previousLocation=loadDriverLocation(deliveryDatabase,id);
    location.statusChangedEpoch=location.updatedEpoch;
    if (!previousLocation && location.status != "ASSIGNED")
        return jsonError(409,"Delivery tracking must start with ASSIGNED");
    if (previousLocation && previousLocation->status != location.status)
    {
        if (deliveryStatusIndex(location.status) != deliveryStatusIndex(previousLocation->status)+1)
            return jsonError(409,"Delivery status must advance one step at a time");
        const int delay=deliveryTransitionDelaySeconds();
        const long long waitUntil=previousLocation->statusChangedEpoch+delay;
        if (location.updatedEpoch < waitUntil)
            return jsonError(409,"Wait "+std::to_string(waitUntil-location.updatedEpoch)+" seconds before the next delivery status");
    }
    else if (previousLocation)
        location.statusChangedEpoch=previousLocation->statusChangedEpoch;
    if (!saveDriverLocation(deliveryDatabase, id, location)) return jsonError(500, "Could not store driver location");
    if (std::string(order["status"].s()) != location.status)
    {
        const auto updateResult = client.updateOrderStatus(id, location.status);
        if (!updateResult.transportSucceeded() || updateResult.statusCode != 200)
            return downstreamResponse(updateResult);
        const auto updated = crow::json::load(updateResult.body);
        if (!updated || !updated.has("success") || !updated["success"].b())
            return jsonError(409, "Order status transition was rejected");
    }
    CROW_LOG_WARNING << "Real driver GPS accepted order=" << id << " accuracyM=" << location.accuracy;
    crow::json::wvalue response; response["success"] = true; response["orderId"] = id;
    response["updatedEpoch"] = location.updatedEpoch; response["status"] = location.status;
    response["nextStatusAllowedEpoch"] = location.statusChangedEpoch + deliveryTransitionDelaySeconds();
    return crow::response(202, response);
});

CROW_ROUTE(app, "/orders/<int>/tracking")
.methods(crow::HTTPMethod::GET)
([&client, &restaurantClient, &paymentClient, &deliveryDatabase](const crow::request& req, int id)
{
    const auto userId = authenticatedUserId(req);
    if (!userId) return unauthorized();
    const auto orderResult = client.getOrderById(id);
    if (!orderResult.transportSucceeded() || orderResult.statusCode != 200)
        return downstreamResponse(orderResult);
    const auto order = crow::json::load(orderResult.body);
    if (!order || !order.has("id")) return jsonError(404, "Order not found");
    if (order["userId"].i() != *userId) return jsonError(403, "This order belongs to another customer");
    const auto paymentResult = paymentClient.getPaymentForOrder(id);
    if (!paymentResult.transportSucceeded() || paymentResult.statusCode != 200)
        return downstreamResponse(paymentResult);
    const auto payment = crow::json::load(paymentResult.body);
    if (!payment || !payment.has("status") || std::string(payment["status"].s()) != "succeeded") {
        CROW_LOG_WARNING << "Tracking rejected order=" << id << " reason=payment-not-succeeded";
        return jsonError(409, "Driver assignment starts only after verified payment");
    }
    CROW_LOG_INFO << "Tracking allowed order=" << id << " payment=succeeded";
    const auto restaurantResult = restaurantClient.getRestaurantById(order["restaurantId"].i());
    if (!restaurantResult.transportSucceeded() || restaurantResult.statusCode != 200)
        return downstreamResponse(restaurantResult);
    const auto restaurant = crow::json::load(restaurantResult.body);
    if (!restaurant || !restaurant.has("latitude")) return jsonError(404, "Restaurant location unavailable");

    const double startLat = restaurant["latitude"].d();
    const double startLon = restaurant["longitude"].d();
    const double endLat = order["deliveryLatitude"].d();
    const double endLon = order["deliveryLongitude"].d();
    auto location = loadDriverLocation(deliveryDatabase, id);
    if (!location) return jsonError(409, "Waiting for the driver to start GPS sharing");
    const long long ageSeconds = std::max(0LL, currentEpoch() - location->updatedEpoch);
    const double totalKm = std::max(0.001, distanceKm(startLat, startLon, endLat, endLon));
    const double remainingKm = distanceKm(location->latitude, location->longitude, endLat, endLon);
    double progress = std::clamp(1.0 - remainingKm / totalKm, 0.0, 1.0);
    std::string deliveryStatus = location->status;
    if (deliveryStatus == "DELIVERED") progress = 1.0;
    const double etaSpeedMps = location->speed >= 0.5 ? location->speed : 5.56;
    const int remainingSeconds = deliveryStatus == "DELIVERED" ? 0 : static_cast<int>(std::ceil(remainingKm * 1000.0 / etaSpeedMps));
    crow::json::wvalue response;
    response["orderId"] = id;
    response["driverId"] = id;
    response["driverName"] = location->name; response["driverContact"] = location->contact;
    response["driverRating"] = 0.0; response["vehicleType"] = location->vehicleType;
    response["vehiclePlate"] = location->vehiclePlate;
    response["driverLatitude"] = location->latitude; response["driverLongitude"] = location->longitude;
    response["accuracyMeters"] = location->accuracy; response["speedMps"] = location->speed;
    response["heading"] = location->heading; response["locationAgeSeconds"] = ageSeconds;
    response["live"] = ageSeconds <= 30;
    response["nextStatusAllowedEpoch"] = location->statusChangedEpoch + deliveryTransitionDelaySeconds();
    response["restaurantLatitude"] = startLat;
    response["restaurantLongitude"] = startLon;
    response["customerLatitude"] = endLat;
    response["customerLongitude"] = endLon;
    response["progressPercent"] = static_cast<int>(progress * 100);
    response["etaMinutes"] = remainingSeconds == 0 ? 0 : static_cast<int>(std::ceil(remainingSeconds / 60.0));
    response["remainingSeconds"] = remainingSeconds;
    response["status"] = deliveryStatus;
    response["lastUpdatedEpoch"] = location->updatedEpoch;
    const std::string stages[] = {"ASSIGNED", "PICKED_UP", "ON_THE_WAY", "ARRIVING", "DELIVERED"};
    int currentStage = deliveryStatus == "ASSIGNED" ? 0 : deliveryStatus == "PICKED_UP" ? 1 :
        deliveryStatus == "ON_THE_WAY" ? 2 : deliveryStatus == "ARRIVING" ? 3 : 4;
    for (int i = 0; i < 5; ++i)
    {
        response["timeline"][i]["status"] = stages[i];
        response["timeline"][i]["complete"] = i <= currentStage;
    }
    response["simulated"] = false;
    return crow::response(response);
});

CROW_ROUTE(app, "/payments").methods(crow::HTTPMethod::POST)
([&paymentClient](const crow::request& req) {
    const auto userId = authenticatedUserId(req);
    if (!userId) return unauthorized();
    const auto input = crow::json::load(req.body);
    if (!input || !input.has("orderId") || !input.has("amount") || !input.has("paymentMethod"))
        return crow::response(400, "Missing orderId, amount, or paymentMethod");
    crow::json::wvalue body;
    body["userId"] = *userId;
    body["orderId"] = input["orderId"].i();
    body["amount"] = input["amount"].d();
    body["paymentMethod"] = input["paymentMethod"].s();
    if (input.has("idempotencyKey")) body["idempotencyKey"] = input["idempotencyKey"].s();
    return downstreamResponse(paymentClient.createPayment(
        body.dump(), req.get_header_value("Idempotency-Key")));
});
CROW_ROUTE(app, "/payments/<int>").methods(crow::HTTPMethod::GET)
([&paymentClient](int id) { return downstreamResponse(paymentClient.getPayment(id)); });
CROW_ROUTE(app, "/payments/order/<int>").methods(crow::HTTPMethod::GET)
([&paymentClient](int id) { return downstreamResponse(paymentClient.getPaymentForOrder(id)); });
CROW_ROUTE(app, "/payments/stream").methods(crow::HTTPMethod::GET)
([&paymentClient](const crow::request& req) {
    const char* id = req.url_params.get("orderId");
    if (!id) return crow::response(422);
    const auto result = paymentClient.getPaymentStream(id);
    if (!result.transportSucceeded() || result.statusCode != 200)
        return downstreamResponse(result);
    crow::response response(static_cast<int>(result.statusCode), result.body);
    response.set_header("Content-Type", "text/event-stream"); response.set_header("Cache-Control", "no-cache");
    return response;
});
CROW_ROUTE(app, "/payments/webhooks/provider").methods(crow::HTTPMethod::POST)
([&paymentClient](const crow::request& req) { return downstreamResponse(paymentClient.providerWebhook(req.body, req.get_header_value("X-Webhook-Secret"))); });
CROW_ROUTE(app, "/payments/razorpay/order").methods(crow::HTTPMethod::POST)
([&paymentClient](const crow::request& req) {
    if (!authenticatedUserId(req)) return unauthorized();
    CROW_LOG_INFO << "Gateway forwarding Razorpay order creation";
    return downstreamResponse(paymentClient.createRazorpayOrder(req.body));
});
CROW_ROUTE(app, "/payments/razorpay/verify").methods(crow::HTTPMethod::POST)
([&paymentClient](const crow::request& req) {
    if (!authenticatedUserId(req)) return unauthorized();
    CROW_LOG_INFO << "Gateway forwarding Razorpay signature verification";
    return downstreamResponse(paymentClient.verifyRazorpayPayment(req.body));
});

CROW_ROUTE(app, "/orders/<int>")
.methods(crow::HTTPMethod::PUT)
([&client](const crow::request& req,
           int id)
{
    return downstreamResponse(client.updateOrder(id, req.body));
});

CROW_ROUTE(app, "/orders/<int>")
.methods(crow::HTTPMethod::DELETE)
([&client, &deliveryDatabase](int id)
{
    const auto existing=client.getOrderById(id);
    if (!existing.transportSucceeded() || existing.statusCode != 200)
        return downstreamResponse(existing);
    const auto order=crow::json::load(existing.body);
    if (order && order.has("status") && std::string(order["status"].s())=="DELIVERED")
        return jsonError(409,"Delivered orders cannot be cancelled");
    const auto result = client.deleteOrder(id);
    removeDriverLocation(deliveryDatabase, id);
    return downstreamResponse(result);
});

    app.loglevel(configuredLogLevel())
       .port(8085)
       .concurrency(128)
       .run();

    return 0;
}
