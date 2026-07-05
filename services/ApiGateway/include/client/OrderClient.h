#pragma once

#include <string>

#include "client/HttpClient.h"

class OrderClient
{
public:

    std::string createOrder(
        const std::string& jsonBody);

    std::string getAllOrders();

    std::string getOrderById(
        int id);

    std::string updateOrder(
        int id,
        const std::string& jsonBody);

    std::string deleteOrder(
        int id);

private:

    HttpClient m_httpClient;
};