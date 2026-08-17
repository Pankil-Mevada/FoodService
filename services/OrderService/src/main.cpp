#include <crow.h>

#include "Database.h"
#include "OrderController.h"
#include "OrderRepository.h"
#include "OrderService.h"

int main()
{
    crow::SimpleApp app;

    // Open database
    Database database("order.db");

    // Create orders table
    database.createOrderTable();

    // Dependency Injection
    OrderRepository repository(database);
    OrderService service(repository);
    OrderController controller(service);

    // Health Check
    CROW_ROUTE(app, "/health")
    ([&controller]()
    {
        return controller.health();
    });

    // Create Order
    CROW_ROUTE(app, "/orders")
        .methods(crow::HTTPMethod::POST)
    ([&controller](const crow::request& req)
    {
        return controller.createOrder(req);
    });

    // Get All Orders
    CROW_ROUTE(app, "/orders")
        .methods(crow::HTTPMethod::GET)
    ([&controller]()
    {
        return controller.getAllOrders();
    });

    // Get Order By Id
    CROW_ROUTE(app, "/orders/<int>")
        .methods(crow::HTTPMethod::GET)
    ([&controller](int id)
    {
        return controller.getOrderById(id);
    });

    // Update Order
    CROW_ROUTE(app, "/orders/<int>")
        .methods(crow::HTTPMethod::PUT)
    ([&controller](const crow::request& req, int id)
    {
        return controller.updateOrder(id, req);
    });

    // Delete Order
    CROW_ROUTE(app, "/orders/<int>")
        .methods(crow::HTTPMethod::DELETE)
    ([&controller](int id)
    {
        return controller.deleteOrder(id);
    });

    // Internal lifecycle update used by the local delivery simulator.
    CROW_ROUTE(app, "/orders/<int>/status")
        .methods(crow::HTTPMethod::POST)
    ([&service](const crow::request& req, int id)
    {
        const auto json = crow::json::load(req.body);
        if (!json || !json.has("status")) return crow::response(400, "Missing status");
        const std::string status(json["status"].s());
        if (status != "ASSIGNED" && status != "PICKED_UP" &&
            status != "ON_THE_WAY" && status != "ARRIVING" && status != "DELIVERED")
            return crow::response(422, "Invalid delivery status");
        const bool updated = service.updateOrderStatus(id, status);
        crow::json::wvalue response;
        response["success"] = updated;
        response["status"] = status;
        return crow::response(updated ? 200 : 404, response);
    });

    app.port(8082)
       .concurrency(128)
       .run();

    return 0;
}
