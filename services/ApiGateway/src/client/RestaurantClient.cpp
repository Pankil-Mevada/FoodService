#include "client/RestaurantClient.h"

std::string RestaurantClient::registerRestaurant(
    const std::string& jsonBody)
{
    return m_httpClient.post(
        "http://localhost:8081/restaurants",
        jsonBody);
}

std::string RestaurantClient::getAllRestaurants()
{
    return m_httpClient.get(
        "http://localhost:8081/restaurants");
}

std::string RestaurantClient::getRestaurantById(
    int id)
{
    return m_httpClient.get(
        "http://localhost:8081/restaurants/" +
        std::to_string(id));
}

std::string RestaurantClient::updateRestaurant(
    int id,
    const std::string& jsonBody)
{
    return m_httpClient.put(
        "http://localhost:8081/restaurants/" +
        std::to_string(id),
        jsonBody);
}

std::string RestaurantClient::deleteRestaurant(
    int id)
{
    return m_httpClient.remove(
        "http://localhost:8081/restaurants/" +
        std::to_string(id));
}