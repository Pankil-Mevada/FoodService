#pragma once

#include <optional>
#include <vector>

#include "Database.h"
#include "Order.h"

class OrderRepository
{
public:

    explicit OrderRepository(Database& database);

    bool saveOrder(const Order& order);

    std::vector<Order> getAllOrders();

    std::optional<Order> getOrderById(int id);

    bool updateOrder(const Order& order);

    bool deleteOrder(int id);

private:

    Database& m_database;
};
