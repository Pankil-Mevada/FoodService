#include <crow.h>

#include "client/OrderClient.h"
#include "client/RestaurantClient.h"
#include "client/UserClient.h"

int main()
{
    crow::SimpleApp app;

    OrderClient orderClient;
    RestaurantClient restaurantClient;
    UserClient userClient;

    // Health
    CROW_ROUTE(app, "/health")
    ([]()
    {
        return "API Gateway is Healthy!";
    });

    // ===========================
    // User APIs
    // ===========================

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
    .methods(crow::HTTPMethod::GET)
    ([&userClient](const crow::request& req)
    {
        return crow::response(
            userClient.getAllUsers(
                req.get_header_value("Authorization")));
    });

    CROW_ROUTE(app, "/users/<int>")
    .methods(crow::HTTPMethod::GET)
    ([&userClient](const crow::request& req, int id)
    {
        return crow::response(
            userClient.getUserById(
                id,
                req.get_header_value("Authorization")));
    });

    CROW_ROUTE(app, "/users/<int>")
    .methods(crow::HTTPMethod::PUT)
    ([&userClient](const crow::request& req, int id)
    {
        return crow::response(
            userClient.updateUser(
                id,
                req.body,
                req.get_header_value("Authorization")));
    });

    CROW_ROUTE(app, "/users/<int>")
    .methods(crow::HTTPMethod::DELETE)
    ([&userClient](const crow::request& req, int id)
    {
        return crow::response(
            userClient.deleteUser(
                id,
                req.get_header_value("Authorization")));
    });

    // ===========================
    // Restaurant APIs
    // ===========================

    CROW_ROUTE(app, "/restaurants")
    .methods(crow::HTTPMethod::GET)
    ([&restaurantClient]()
    {
        return crow::response(
            restaurantClient.getAllRestaurants());
    });

    CROW_ROUTE(app, "/restaurants")
    .methods(crow::HTTPMethod::POST)
    ([&restaurantClient](const crow::request& req)
    {
        return crow::response(
            restaurantClient.registerRestaurant(req.body));
    });

    CROW_ROUTE(app, "/restaurants/<int>")
    .methods(crow::HTTPMethod::GET)
    ([&restaurantClient](int id)
    {
        return crow::response(
            restaurantClient.getRestaurantById(id));
    });

    CROW_ROUTE(app, "/restaurants/<int>")
    .methods(crow::HTTPMethod::PUT)
    ([&restaurantClient](const crow::request& req, int id)
    {
        return crow::response(
            restaurantClient.updateRestaurant(id, req.body));
    });

    CROW_ROUTE(app, "/restaurants/<int>")
    .methods(crow::HTTPMethod::DELETE)
    ([&restaurantClient](int id)
    {
        return crow::response(
            restaurantClient.deleteRestaurant(id));
    });

    // ===========================
    // Order APIs
    // ===========================

    CROW_ROUTE(app, "/orders")
    .methods(crow::HTTPMethod::POST)
    ([&orderClient](const crow::request& req)
    {
        return crow::response(
            orderClient.createOrder(req.body));
    });

    CROW_ROUTE(app, "/orders")
    .methods(crow::HTTPMethod::GET)
    ([&orderClient]()
    {
        return crow::response(
            orderClient.getAllOrders());
    });

    CROW_ROUTE(app, "/orders/<int>")
    .methods(crow::HTTPMethod::GET)
    ([&orderClient](int id)
    {
        return crow::response(
            orderClient.getOrderById(id));
    });

    CROW_ROUTE(app, "/orders/<int>")
    .methods(crow::HTTPMethod::PUT)
    ([&orderClient](const crow::request& req, int id)
    {
        return crow::response(
            orderClient.updateOrder(id, req.body));
    });

    CROW_ROUTE(app, "/orders/<int>")
    .methods(crow::HTTPMethod::DELETE)
    ([&orderClient](int id)
    {
        return crow::response(
            orderClient.deleteOrder(id));
    });

    app.port(8085)
       .multithreaded()
       .run();

    return 0;
}