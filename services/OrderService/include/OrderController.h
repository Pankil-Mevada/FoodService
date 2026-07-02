#pragma once

#include <crow.h>

#include "OrderService.h"

class OrderController
{
public:

    explicit OrderController(OrderService& service);

    crow::response health();

    crow::response createOrder(const crow::request& req);

    crow::response getAllOrders();

    crow::response getOrderById(int id);

    crow::response updateOrder(
        int id,
        const crow::request& req);

    crow::response deleteOrder(int id);

private:

    OrderService& m_service;
};
