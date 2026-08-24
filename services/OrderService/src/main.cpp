#include <crow.h>

#include "Database.h"
#include "OrderController.h"
#include "OrderRepository.h"
#include "OrderService.h"

#include <algorithm>
#include <cstdlib>

namespace
{
bool safeEqual(const std::string& left, const std::string& right)
{
    size_t difference = left.size() ^ right.size();
    for (size_t index = 0; index < std::max(left.size(), right.size()); ++index)
        difference |= static_cast<unsigned char>(index < left.size() ? left[index] : 0) ^
            static_cast<unsigned char>(index < right.size() ? right[index] : 0);
    return difference == 0;
}

bool internalOrderSyncAuthorized(const crow::request& req)
{
    const char* configured = std::getenv("ORDER_SYNC_SECRET");
    const std::string expected = configured && *configured
        ? configured : "local-order-sync-secret";
    return safeEqual(req.get_header_value("X-Internal-Secret"), expected);
}
}

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

    // Internal callback used by Payment Service after a verified provider event.
    CROW_ROUTE(app, "/orders/<int>/payment-status")
        .methods(crow::HTTPMethod::POST)
    ([&service](const crow::request& req, int id)
    {
        if (!internalOrderSyncAuthorized(req))
            return crow::response(401, "Invalid internal order-sync secret");
        const auto json = crow::json::load(req.body);
        if (!json || !json.has("paymentStatus"))
            return crow::response(400, "paymentStatus is required");
        const std::string paymentStatus(json["paymentStatus"].s());
        if (paymentStatus != "processing" && paymentStatus != "succeeded" &&
            paymentStatus != "failed" && paymentStatus != "cancelled")
            return crow::response(422, "Unsupported payment status");
        const bool updated = service.updateOrderPaymentStatus(id, paymentStatus);
        crow::json::wvalue response;
        response["success"] = updated;
        response["paymentStatus"] = paymentStatus;
        const auto order = service.getOrderById(id);
        if (order) response["orderStatus"] = order->getStatus();
        return crow::response(updated ? 200 : 409, response);
    });

    app.port(8082)
       .concurrency(128)
       .run();

    return 0;
}
