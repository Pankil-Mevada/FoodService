#include "OrderService.h"
#include <iostream>

OrderService::OrderService(OrderRepository& repository)
    : m_repository(repository)
{
}


bool OrderService::createOrder(const Order& order)
{
    std::clog << "[order-flow] create start user=" << order.getUserId()
              << " restaurant=" << order.getRestaurantId() << " amount=" << order.getTotalAmount() << std::endl;
    bool restaurantFound =
        m_restaurantClient.restaurantExists(
            order.getRestaurantId());

    if (!restaurantFound)
    {
        std::clog << "[order-flow] create rejected reason=restaurant-not-found" << std::endl;
        return false;
    }

    auto orderId =
    m_repository.saveOrder(order);

    if (!orderId.has_value())
    {
        std::clog << "[order-flow] create failed stage=database-save" << std::endl;
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
            "PAYMENT_PENDING");
        std::clog << "[order-flow] create accepted order=" << *orderId << " status=PAYMENT_PENDING" << std::endl;
    }
else
{
    m_repository.updateOrderStatus(
    *orderId,
    "PAYMENT_FAILED");
    std::clog << "[order-flow] create failed order=" << *orderId << " stage=payment-record" << std::endl;
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
    const bool deliveryStatus = status == "ASSIGNED" || status == "PICKED_UP" ||
        status == "ON_THE_WAY" || status == "ARRIVING" || status == "DELIVERED";
    if (deliveryStatus && !m_paymentClient.isPaymentSucceeded(orderId)) {
        std::clog << "[delivery-flow] transition rejected order=" << orderId
                  << " nextStatus=" << status << " reason=payment-not-succeeded" << std::endl;
        return false;
    }
    if (deliveryStatus)
        std::clog << "[delivery-flow] transition allowed order=" << orderId
                  << " nextStatus=" << status << std::endl;
    return m_repository.updateOrderStatus(
        orderId,
        status);
}
