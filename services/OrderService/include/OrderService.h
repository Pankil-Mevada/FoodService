#pragma once

#include <optional>
#include <vector>

#include "Order.h"
#include "OrderRepository.h"

class OrderService
{
public:

    explicit OrderService(OrderRepository& repository);

    bool createOrder(const Order& order);

    std::vector<Order> getAllOrders();

    std::optional<Order> getOrderById(int id);

    bool updateOrder(const Order& order);

    bool deleteOrder(int id);

private:

    OrderRepository& m_repository;
};
