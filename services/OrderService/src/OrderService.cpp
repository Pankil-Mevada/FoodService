#include "OrderService.h"

OrderService::OrderService(OrderRepository& repository)
    : m_repository(repository)
{
}


bool OrderService::createOrder(const Order& order)
{
    bool restaurantFound =
        m_restaurantClient.restaurantExists(
            order.getRestaurantId());

    if (!restaurantFound)
    {
        return false;
    }

    auto orderId =
    m_repository.saveOrder(order);

    if (!orderId.has_value())
    {
        return false;
    }

    bool paymentStatus =
    m_paymentClient.createPayment(
    *orderId,
    order.getUserId(),
    order.getTotalAmount());

    if (paymentStatus)
    {
        m_repository.updateOrderStatus(
            *orderId,
            "PAID");
    }
else
{
    m_repository.updateOrderStatus(
    *orderId,
    "PAYMENT_FAILED");
}

return paymentStatus;
}
std::vector<Order> OrderService::getAllOrders()
{
    return m_repository.getAllOrders();
}

std::optional<Order> OrderService::getOrderById(int id)
{
    return m_repository.getOrderById(id);
}

bool OrderService::updateOrder(const Order& order)
{
    return m_repository.updateOrder(order);
}

bool OrderService::deleteOrder(int id)
{
    return m_repository.deleteOrder(id);
}

bool OrderService::updateOrderStatus(
    int orderId,
    const std::string& status)
{
    return m_repository.updateOrderStatus(
        orderId,
        status);
}