#include "client/OrderClient.h"

#include <curl/curl.h>

namespace
{
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

std::string OrderClient::createOrder(
    const std::string& jsonBody)
{
       return m_httpClient.post(
        "http://localhost:8082/orders",
        jsonBody);
}

std::string OrderClient::getAllOrders()
{
     return m_httpClient.get(
        "http://localhost:8082/orders");
}

std::string OrderClient::getOrderById(int id)
{
 return m_httpClient.get(
        "http://localhost:8082/orders/" +
        std::to_string(id));
}


std::string OrderClient::updateOrder(
    int id,
    const std::string& jsonBody)
{
    return m_httpClient.put(
        "http://localhost:8082/orders/" +
        std::to_string(id),
        jsonBody);
}

std::string OrderClient::deleteOrder(
    int id)
{
return m_httpClient.remove(
        "http://localhost:8082/orders/" +
        std::to_string(id));
}