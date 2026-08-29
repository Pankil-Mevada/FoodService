#pragma once

#include <string>

#include "client/HttpClient.h"

class OrderClient
{
public:

    HttpResult createOrder(
        const std::string& jsonBody);

    HttpResult getAllOrders();

    HttpResult getOrderById(
        int id);

    HttpResult updateOrder(
        int id,
        const std::string& jsonBody);

    HttpResult deleteOrder(
        int id);

    HttpResult updateOrderStatus(
        int id,
        const std::string& status);

private:

    HttpClient m_httpClient;
};
