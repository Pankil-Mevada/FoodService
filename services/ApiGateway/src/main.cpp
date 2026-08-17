#include <crow.h>
#include <crow/middlewares/cors.h>
#include "client/OrderClient.h"
#include "client/RestaurantClient.h"
#include "client/UserClient.h"
#include "client/PaymentClient.h"
#include "JwtManager.h"
#include <chrono>
#include <cmath>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

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
}

int main()
{
    crow::App<crow::CORSHandler> app;

    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .origin("*")
        .headers("Content-Type", "Authorization", "Idempotency-Key", "X-Webhook-Secret")
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
    return crow::response(
        userClient.registerUser(req.body));
});

CROW_ROUTE(app, "/login")
.methods(crow::HTTPMethod::POST)
([&userClient](const crow::request& req)
{
    return crow::response(
        userClient.login(req.body));
});

CROW_ROUTE(app, "/me")
.methods(crow::HTTPMethod::GET)
([&userClient](const crow::request& req)
{
    const auto userId = authenticatedUserId(req);
    if (!userId) return unauthorized();
    return crow::response(userClient.getUserById(
        *userId, req.get_header_value("Authorization")));
});

CROW_ROUTE(app, "/users")
([&userClient](const crow::request& req)
{
    switch (req.method)
    {
        case crow::HTTPMethod::GET:
            return crow::response(
                userClient.getAllUsers(
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
            return crow::response(
                userClient.getUserById(
    id,
    req.get_header_value("Authorization")));

        case crow::HTTPMethod::PUT:
            return crow::response(
                userClient.updateUser(
    id,
    req.body,
    req.get_header_value("Authorization")));

        case crow::HTTPMethod::DELETE:
            return crow::response(
                userClient.deleteUser(
    id,
    req.get_header_value("Authorization")));

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
            return crow::response(
                restaurantClient.getAllRestaurants());

        case crow::HTTPMethod::POST:
            return crow::response(
                restaurantClient.registerRestaurant(req.body));

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
            return crow::response(
                restaurantClient.getRestaurantById(id));

        case crow::HTTPMethod::PUT:
            return crow::response(
                restaurantClient.updateRestaurant(id, req.body));

        case crow::HTTPMethod::DELETE:
            return crow::response(
                restaurantClient.deleteRestaurant(id));

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

    CROW_ROUTE(app, "/orders")
        .methods(crow::HTTPMethod::POST)
    ([&client, &restaurantClient](const crow::request& req)
    {
        const auto userId = authenticatedUserId(req);
        if (!userId) return unauthorized();
        const auto input = crow::json::load(req.body);
        if (!input || !input.has("restaurantId") || !input.has("totalAmount") ||
            !input.has("deliveryLatitude") || !input.has("deliveryLongitude") || !input.has("deliveryAddress"))
            return jsonError(400, "Restaurant, amount, and delivery location are required");
        const double deliveryLat = input["deliveryLatitude"].d();
        const double deliveryLon = input["deliveryLongitude"].d();
        if (deliveryLat < -90 || deliveryLat > 90 || deliveryLon < -180 || deliveryLon > 180)
            return jsonError(422, "Delivery coordinates are invalid");
        const auto restaurant = crow::json::load(restaurantClient.getRestaurantById(input["restaurantId"].i()));
        if (!restaurant || !restaurant.has("latitude") || !restaurant.has("longitude"))
            return jsonError(404, "Restaurant location is unavailable");
        const double distance = distanceKm(deliveryLat, deliveryLon,
            restaurant["latitude"].d(), restaurant["longitude"].d());
        const double radius = restaurant.has("deliveryRadiusKm") ? restaurant["deliveryRadiusKm"].d() : 8.0;
        if (distance > radius)
            return jsonError(422, "This address is outside the restaurant delivery area");
        crow::json::wvalue body;
        body["userId"] = *userId;
        body["restaurantId"] = input["restaurantId"].i();
        body["totalAmount"] = input["totalAmount"].d();
        body["deliveryLatitude"] = deliveryLat;
        body["deliveryLongitude"] = deliveryLon;
        body["deliveryAddress"] = input["deliveryAddress"].s();
        body["itemSummary"] = input.has("itemSummary") ? std::string(input["itemSummary"].s()) : "";
        body["subtotal"] = input.has("subtotal") ? input["subtotal"].d() : input["totalAmount"].d();
        body["discountAmount"] = input.has("discountAmount") ? input["discountAmount"].d() : 0.0;
        body["deliveryFee"] = input.has("deliveryFee") ? input["deliveryFee"].d() : 0.0;
        return crow::response(client.createOrder(body.dump()));
    });

CROW_ROUTE(app, "/orders")
.methods(crow::HTTPMethod::GET)
([&client](const crow::request& req)
{
    const auto userId = authenticatedUserId(req);
    if (!userId) return unauthorized();
    const auto orders = crow::json::load(client.getAllOrders());
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
    return crow::response(
        client.getOrderById(id));
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

    const auto provider = crow::json::load(restaurantClient.discoverNearby(latitude, longitude));
    if (!provider || !provider.has("elements"))
        return jsonError(503, "Nearby restaurant provider is temporarily unavailable");

    std::unordered_set<std::string> existingNames;
    const auto existing = crow::json::load(restaurantClient.getAllRestaurants());
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
        restaurantClient.registerRestaurant(body.dump());
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

CROW_ROUTE(app, "/orders/<int>/tracking")
.methods(crow::HTTPMethod::GET)
([&client, &restaurantClient, &paymentClient](const crow::request& req, int id)
{
    const auto userId = authenticatedUserId(req);
    if (!userId) return unauthorized();
    const auto order = crow::json::load(client.getOrderById(id));
    if (!order || !order.has("id")) return jsonError(404, "Order not found");
    if (order["userId"].i() != *userId) return jsonError(403, "This order belongs to another customer");
    const auto payment = crow::json::load(paymentClient.getPaymentForOrder(id));
    if (!payment || !payment.has("status") || std::string(payment["status"].s()) != "succeeded") {
        CROW_LOG_WARNING << "Tracking rejected order=" << id << " reason=payment-not-succeeded";
        return jsonError(409, "Driver assignment starts only after verified payment");
    }
    CROW_LOG_INFO << "Tracking allowed order=" << id << " payment=succeeded";
    const auto restaurant = crow::json::load(restaurantClient.getRestaurantById(order["restaurantId"].i()));
    if (!restaurant || !restaurant.has("latitude")) return jsonError(404, "Restaurant location unavailable");

    const double startLat = restaurant["latitude"].d();
    const double startLon = restaurant["longitude"].d();
    const double endLat = order["deliveryLatitude"].d();
    const double endLon = order["deliveryLongitude"].d();
    static std::mutex trackingMutex;
    static std::unordered_map<int, std::chrono::steady_clock::time_point> trackingStarted;
    const auto now = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point started;
    {
        std::lock_guard<std::mutex> lock(trackingMutex);
        auto [entry, inserted] = trackingStarted.emplace(id, now);
        started = entry->second;
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - started).count();
    if (std::string(order["status"].s()) == "DELIVERED") elapsed = 180;
    double progress = 0.05;
    std::string deliveryStatus = "ASSIGNED";
    if (elapsed >= 180) { progress = 1.0; deliveryStatus = "DELIVERED"; }
    else if (elapsed >= 135) { progress = 0.75 + ((elapsed - 135) / 45.0) * 0.25; deliveryStatus = "ARRIVING"; }
    else if (elapsed >= 45) { progress = 0.25 + ((elapsed - 45) / 90.0) * 0.50; deliveryStatus = "ON_THE_WAY"; }
    else if (elapsed >= 15) { progress = 0.15 + ((elapsed - 15) / 30.0) * 0.10; deliveryStatus = "PICKED_UP"; }
    if (std::string(order["status"].s()) != deliveryStatus)
        client.updateOrderStatus(id, deliveryStatus);
    CROW_LOG_INFO << "Tracking snapshot order=" << id << " status=" << deliveryStatus
                  << " progress=" << static_cast<int>(progress * 100);
    const int remainingSeconds = std::max(0, 180 - static_cast<int>(elapsed));
    crow::json::wvalue response;
    response["orderId"] = id;
    response["driverId"] = 1000 + (id % 7);
    response["driverName"] = std::string("Delivery Partner ") + char('A' + (id % 7));
    response["driverContact"] = std::string("TEST-DRIVER-") + std::to_string(1000 + (id % 7));
    response["driverRating"] = 4.8;
    response["vehicleType"] = "Electric test scooter";
    response["vehiclePlate"] = std::string("TEST-KA-") + std::to_string(1000 + (id % 9000));
    response["driverLatitude"] = startLat + (endLat - startLat) * progress;
    response["driverLongitude"] = startLon + (endLon - startLon) * progress;
    response["restaurantLatitude"] = startLat;
    response["restaurantLongitude"] = startLon;
    response["customerLatitude"] = endLat;
    response["customerLongitude"] = endLon;
    response["progressPercent"] = static_cast<int>(progress * 100);
    response["etaMinutes"] = remainingSeconds == 0 ? 0 : static_cast<int>(std::ceil(remainingSeconds / 60.0));
    response["remainingSeconds"] = remainingSeconds;
    response["status"] = deliveryStatus;
    response["lastUpdatedEpoch"] = static_cast<long long>(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    const std::string stages[] = {"ASSIGNED", "PICKED_UP", "ON_THE_WAY", "ARRIVING", "DELIVERED"};
    int currentStage = deliveryStatus == "ASSIGNED" ? 0 : deliveryStatus == "PICKED_UP" ? 1 :
        deliveryStatus == "ON_THE_WAY" ? 2 : deliveryStatus == "ARRIVING" ? 3 : 4;
    for (int i = 0; i < 5; ++i)
    {
        response["timeline"][i]["status"] = stages[i];
        response["timeline"][i]["complete"] = i <= currentStage;
    }
    response["simulated"] = true;
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
    return crow::response(paymentClient.createPayment(body.dump(), req.get_header_value("Idempotency-Key")));
});
CROW_ROUTE(app, "/payments/<int>").methods(crow::HTTPMethod::GET)
([&paymentClient](int id) { return crow::response(paymentClient.getPayment(id)); });
CROW_ROUTE(app, "/payments/order/<int>").methods(crow::HTTPMethod::GET)
([&paymentClient](int id) { return crow::response(paymentClient.getPaymentForOrder(id)); });
CROW_ROUTE(app, "/payments/stream").methods(crow::HTTPMethod::GET)
([&paymentClient](const crow::request& req) {
    const char* id = req.url_params.get("orderId");
    if (!id) return crow::response(422);
    crow::response response(paymentClient.getPaymentStream(id));
    response.set_header("Content-Type", "text/event-stream"); response.set_header("Cache-Control", "no-cache");
    return response;
});
CROW_ROUTE(app, "/payments/webhooks/provider").methods(crow::HTTPMethod::POST)
([&paymentClient](const crow::request& req) { return crow::response(paymentClient.providerWebhook(req.body, req.get_header_value("X-Webhook-Secret"))); });
CROW_ROUTE(app, "/payments/razorpay/order").methods(crow::HTTPMethod::POST)
([&paymentClient](const crow::request& req) {
    if (!authenticatedUserId(req)) return unauthorized();
    CROW_LOG_INFO << "Gateway forwarding Razorpay order creation";
    return crow::response(paymentClient.createRazorpayOrder(req.body));
});
CROW_ROUTE(app, "/payments/razorpay/verify").methods(crow::HTTPMethod::POST)
([&paymentClient](const crow::request& req) {
    if (!authenticatedUserId(req)) return unauthorized();
    CROW_LOG_INFO << "Gateway forwarding Razorpay signature verification";
    return crow::response(paymentClient.verifyRazorpayPayment(req.body));
});

CROW_ROUTE(app, "/orders/<int>")
.methods(crow::HTTPMethod::PUT)
([&client](const crow::request& req,
           int id)
{
    return crow::response(
        client.updateOrder(
            id,
            req.body));
});

CROW_ROUTE(app, "/orders/<int>")
.methods(crow::HTTPMethod::DELETE)
([&client](int id)
{
    return crow::response(
        client.deleteOrder(id));
});

    app.loglevel(crow::LogLevel::Warning)
       .port(8085)
       .concurrency(128)
       .run();

    return 0;
}
