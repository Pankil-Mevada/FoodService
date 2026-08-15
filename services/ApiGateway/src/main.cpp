#include <crow.h>
#include <crow/middlewares/cors.h>
#include "client/OrderClient.h"
#include "client/RestaurantClient.h"
#include "client/UserClient.h"
#include "client/PaymentClient.h"
#include "JwtManager.h"

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
    ([&client](const crow::request& req)
    {
        const auto userId = authenticatedUserId(req);
        if (!userId) return unauthorized();
        const auto input = crow::json::load(req.body);
        if (!input || !input.has("restaurantId") || !input.has("totalAmount"))
            return crow::response(400, "Missing restaurantId or totalAmount");
        crow::json::wvalue body;
        body["userId"] = *userId;
        body["restaurantId"] = input["restaurantId"].i();
        body["totalAmount"] = input["totalAmount"].d();
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
