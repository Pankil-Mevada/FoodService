#include "OrderController.h"
#include <cmath>

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
         "PENDING",
         json.has("deliveryLatitude") ? json["deliveryLatitude"].d() : 0.0,
         json.has("deliveryLongitude") ? json["deliveryLongitude"].d() : 0.0,
         json.has("deliveryAddress") ? std::string(json["deliveryAddress"].s()) : std::string(),
         json.has("itemSummary") ? std::string(json["itemSummary"].s()) : std::string(),
         json.has("subtotal") ? json["subtotal"].d() : json["totalAmount"].d(),
         json.has("discountAmount") ? json["discountAmount"].d() : 0.0,
         json.has("deliveryFee") ? json["deliveryFee"].d() : 0.0);

    if (order.getSubtotal() < 0 || order.getDiscountAmount() < 0 ||
        order.getDiscountAmount() > order.getSubtotal() || order.getDeliveryFee() < 0 ||
        std::abs(order.getTotalAmount() - (order.getSubtotal() - order.getDiscountAmount() + order.getDeliveryFee())) > 0.01)
        return crow::response(422, "Order price breakdown is invalid");

    const auto orderId = m_service.createOrder(order);

    crow::json::wvalue response;

    if (orderId)
    {
        response["success"] = true;
        response["message"] = "Order created successfully";
        response["orderId"] = *orderId;

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
        response[index]["deliveryLatitude"] = order.getDeliveryLatitude();
        response[index]["deliveryLongitude"] = order.getDeliveryLongitude();
        response[index]["deliveryAddress"] = order.getDeliveryAddress();
        response[index]["itemSummary"] = order.getItemSummary();
        response[index]["subtotal"] = order.getSubtotal();
        response[index]["discountAmount"] = order.getDiscountAmount();
        response[index]["deliveryFee"] = order.getDeliveryFee();

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
    response["deliveryLatitude"] = order->getDeliveryLatitude();
    response["deliveryLongitude"] = order->getDeliveryLongitude();
    response["deliveryAddress"] = order->getDeliveryAddress();
    response["itemSummary"] = order->getItemSummary();
    response["subtotal"] = order->getSubtotal();
    response["discountAmount"] = order->getDiscountAmount();
    response["deliveryFee"] = order->getDeliveryFee();

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
     "PENDING",
     json.has("deliveryLatitude") ? json["deliveryLatitude"].d() : 0.0,
     json.has("deliveryLongitude") ? json["deliveryLongitude"].d() : 0.0,
     json.has("deliveryAddress") ? std::string(json["deliveryAddress"].s()) : std::string(),
     json.has("itemSummary") ? std::string(json["itemSummary"].s()) : std::string(),
     json.has("subtotal") ? json["subtotal"].d() : json["totalAmount"].d(),
     json.has("discountAmount") ? json["discountAmount"].d() : 0.0,
     json.has("deliveryFee") ? json["deliveryFee"].d() : 0.0);

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
