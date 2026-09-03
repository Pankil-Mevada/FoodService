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

    HttpResult getPartnerOrders(
        int restaurantId,
        int partnerUserId,
        const std::string& partnerRole);

    HttpResult updatePartnerOrderStatus(
        int restaurantId,
        int orderId,
        int partnerUserId,
        const std::string& partnerRole,
        const std::string& jsonBody,
        const std::string& idempotencyKey);

private:

    HttpClient m_httpClient;
};
