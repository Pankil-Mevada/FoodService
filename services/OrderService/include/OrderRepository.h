#pragma once

#include <optional>
#include <vector>

#include "Database.h"
#include "Order.h"

class OrderRepository
{
public:

    explicit OrderRepository(Database& database);

   std::optional<int> saveOrder(const Order& order);

    std::vector<Order> getAllOrders();

    std::optional<Order> getOrderById(int id);

    bool updateOrder(const Order& order);

    bool deleteOrder(int id);

    bool updateOrderStatus(
    int orderId,
    const std::string& status);

private:

    Database& m_database;
};
