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

    app.port(8082)
       .multithreaded()
       .run();

    return 0;
}
