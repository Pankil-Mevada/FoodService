#include "client/RestaurantClient.h"
#include <curl/curl.h>
#include <cstdlib>

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

std::string RestaurantClient::discoverNearby(double latitude, double longitude)
{
    const std::string query = "[out:json][timeout:15];nwr(around:5000," +
        std::to_string(latitude) + "," + std::to_string(longitude) +
        ")[\"amenity\"=\"restaurant\"][\"name\"];out center tags 20;";
    CURL* curl = curl_easy_init();
    if (!curl) return {};
    char* encoded = curl_easy_escape(curl, query.c_str(), static_cast<int>(query.size()));
    const char* configured = std::getenv("FOODSERVICE_OVERPASS_URL");
    const std::string endpoint = configured && *configured ? configured :
        "https://overpass-api.de/api/interpreter";
    const std::string url = endpoint + "?data=" +
        std::string(encoded ? encoded : "");
    if (encoded) curl_free(encoded);
    curl_easy_cleanup(curl);
    return m_httpClient.get(url);
}
