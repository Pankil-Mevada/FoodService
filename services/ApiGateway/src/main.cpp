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
        return crow::response(client.createOrder(body.dump()));
    });

CROW_ROUTE(app, "/orders")
.methods(crow::HTTPMethod::GET)
([&client]()
{
    return crow::response(
        client.getAllOrders());
});

CROW_ROUTE(app, "/orders/<int>")
.methods(crow::HTTPMethod::GET)
([&client](int id)
{
    return crow::response(
        client.getOrderById(id));
});

CROW_ROUTE(app, "/orders/<int>/tracking")
.methods(crow::HTTPMethod::GET)
([&client, &restaurantClient](const crow::request& req, int id)
{
    const auto userId = authenticatedUserId(req);
    if (!userId) return unauthorized();
    const auto order = crow::json::load(client.getOrderById(id));
    if (!order || !order.has("id")) return jsonError(404, "Order not found");
    if (order["userId"].i() != *userId) return jsonError(403, "This order belongs to another customer");
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
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - started).count();
    const double progress = std::min(0.95, 0.15 + (elapsed / 5) * 0.05);
    const double remainingKm = distanceKm(startLat, startLon, endLat, endLon) * (1.0 - progress);
    crow::json::wvalue response;
    response["orderId"] = id;
    response["driverId"] = 1000 + (id % 7);
    response["driverName"] = std::string("Delivery Partner ") + char('A' + (id % 7));
    response["driverLatitude"] = startLat + (endLat - startLat) * progress;
    response["driverLongitude"] = startLon + (endLon - startLon) * progress;
    response["restaurantLatitude"] = startLat;
    response["restaurantLongitude"] = startLon;
    response["customerLatitude"] = endLat;
    response["customerLongitude"] = endLon;
    response["progressPercent"] = static_cast<int>(progress * 100);
    response["etaMinutes"] = std::max(1, static_cast<int>(std::ceil(remainingKm / 0.35)));
    response["status"] = progress > 0.8 ? "ARRIVING" : "ON_THE_WAY";
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

    app.port(8085)
       .multithreaded()
       .run();

    return 0;
}
