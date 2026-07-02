#include "OrderController.h"

OrderController::OrderController(OrderService& service)
    : m_service(service)
{
}

crow::response OrderController::health()
{
    return crow::response("Order Service is Healthy!");
}

crow::response OrderController::createOrder(
    const crow::request& req)
{
    auto json = crow::json::load(req.body);

    if (!json)
    {
        return crow::response(400, "Invalid JSON");
    }

    Order order(
        0,
        json["userId"].i(),
        json["restaurantId"].i(),
        json["totalAmount"].d(),
        json["status"].s());

    bool status = m_service.createOrder(order);

    crow::json::wvalue response;

    if (status)
    {
        response["success"] = true;
        response["message"] = "Order created successfully";

        return crow::response(201, response);
    }

    response["success"] = false;
    response["message"] = "Failed to create order";

    return crow::response(500, response);
}

crow::response OrderController::getAllOrders()
{
    auto orders = m_service.getAllOrders();

    crow::json::wvalue response;

    size_t index = 0;

    for (const auto& order : orders)
    {
        response[index]["id"] = order.getId();
        response[index]["userId"] = order.getUserId();
        response[index]["restaurantId"] = order.getRestaurantId();
        response[index]["totalAmount"] = order.getTotalAmount();
        response[index]["status"] = order.getStatus();

        ++index;
    }

    return crow::response(response);
}

crow::response OrderController::getOrderById(int id)
{
    auto order = m_service.getOrderById(id);

    if (!order.has_value())
    {
        crow::json::wvalue response;

        response["success"] = false;
        response["message"] = "Order not found";

        return crow::response(404, response);
    }

    crow::json::wvalue response;

    response["id"] = order->getId();
    response["userId"] = order->getUserId();
    response["restaurantId"] = order->getRestaurantId();
    response["totalAmount"] = order->getTotalAmount();
    response["status"] = order->getStatus();

    return crow::response(response);
}

crow::response OrderController::updateOrder(
    int id,
    const crow::request& req)
{
    auto json = crow::json::load(req.body);

    if (!json)
    {
        return crow::response(400, "Invalid JSON");
    }

    Order order(
        id,
        json["userId"].i(),
        json["restaurantId"].i(),
        json["totalAmount"].d(),
        json["status"].s());

    bool status = m_service.updateOrder(order);

    crow::json::wvalue response;

    if (status)
    {
        response["success"] = true;
        response["message"] = "Order updated successfully";
    }
    else
    {
        response["success"] = false;
        response["message"] = "Order not found";
    }

    return crow::response(response);
}

crow::response OrderController::deleteOrder(int id)
{
    bool status = m_service.deleteOrder(id);

    crow::json::wvalue response;

    if (status)
    {
        response["success"] = true;
        response["message"] = "Order deleted successfully";
    }
    else
    {
        response["success"] = false;
        response["message"] = "Order not found";
    }

    return crow::response(response);
}
