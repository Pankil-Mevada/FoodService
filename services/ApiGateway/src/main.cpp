#include <crow.h>
#include <crow/middlewares/cors.h>
#include "client/OrderClient.h"
#include "client/RestaurantClient.h"
#include "client/UserClient.h"
#include "client/PaymentClient.h"
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
        return crow::response(
            client.createOrder(req.body));
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
([&paymentClient](const crow::request& req) { return crow::response(paymentClient.createPayment(req.body, req.get_header_value("Idempotency-Key"))); });
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
