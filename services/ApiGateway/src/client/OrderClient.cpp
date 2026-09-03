#include "client/OrderClient.h"

#include <curl/curl.h>
#include <cstdlib>
#include <vector>

namespace
{
std::string internalOrderSecret()
{
    const char* configured = std::getenv("ORDER_SYNC_SECRET");
    return configured && *configured ? configured : "local-order-sync-secret";
}

std::string orderServiceUrl()
{
    const char* configured = std::getenv("ORDER_SERVICE_URL");
    return configured && *configured ? configured : "http://localhost:8082";
}

std::vector<std::string> partnerHeaders(int userId, const std::string& role)
{
    return {
        "X-Internal-Secret: " + internalOrderSecret(),
        "X-Partner-User-ID: " + std::to_string(userId),
        "X-Partner-Role: " + role
    };
}

size_t WriteCallback(
    void* contents,
    size_t size,
    size_t nmemb,
    void* userp)
{
    ((std::string*)userp)->append(
        (char*)contents,
        size * nmemb);

    return size * nmemb;
}
}

HttpResult OrderClient::createOrder(
    const std::string& jsonBody)
{
       return m_httpClient.post(
        "http://localhost:8082/orders",
        jsonBody);
}

HttpResult OrderClient::getAllOrders()
{
     return m_httpClient.get(
        "http://localhost:8082/orders");
}

HttpResult OrderClient::getOrderById(int id)
{
 return m_httpClient.get(
        "http://localhost:8082/orders/" +
        std::to_string(id));
}


HttpResult OrderClient::updateOrder(
    int id,
    const std::string& jsonBody)
{
    return m_httpClient.put(
        "http://localhost:8082/orders/" +
        std::to_string(id),
        jsonBody);
}

HttpResult OrderClient::deleteOrder(
    int id)
{
return m_httpClient.remove(
        "http://localhost:8082/orders/" +
        std::to_string(id));
}

HttpResult OrderClient::updateOrderStatus(int id, const std::string& status)
{
    return m_httpClient.post(
        "http://localhost:8082/orders/" + std::to_string(id) + "/status",
        "{\"status\":\"" + status + "\"}");
}

HttpResult OrderClient::getPartnerOrders(
    int restaurantId,
    int partnerUserId,
    const std::string& partnerRole)
{
    return m_httpClient.get(
        orderServiceUrl() + "/internal/partner/restaurants/" +
            std::to_string(restaurantId) + "/orders",
        "", partnerHeaders(partnerUserId, partnerRole));
}

HttpResult OrderClient::updatePartnerOrderStatus(
    int restaurantId,
    int orderId,
    int partnerUserId,
    const std::string& partnerRole,
    const std::string& jsonBody,
    const std::string& idempotencyKey)
{
    auto headers = partnerHeaders(partnerUserId, partnerRole);
    headers.push_back("Idempotency-Key: " + idempotencyKey);
    return m_httpClient.post(
        orderServiceUrl() + "/internal/partner/restaurants/" +
            std::to_string(restaurantId) + "/orders/" +
            std::to_string(orderId) + "/status",
        jsonBody, "", headers);
}
