#pragma once

#include <optional>
#include <vector>

#include "Order.h"
#include "OrderRepository.h"
#include "client/PaymentClient.h"
#include "client/RestaurantClient.h"
class OrderService
{
public:

    explicit OrderService(OrderRepository& repository);

    std::optional<int> createOrder(const Order& order);

    std::vector<Order> getAllOrders();

    std::optional<Order> getOrderById(int id);

    bool updateOrder(const Order& order);

    bool deleteOrder(int id);

    bool updateOrderStatus(
    int orderId,
    const std::string& status);

    bool updateOrderPaymentStatus(
        int orderId,
        const std::string& paymentStatus);

private:

    OrderRepository& m_repository;
    PaymentClient m_paymentClient;
    RestaurantClient m_restaurantClient;
};
