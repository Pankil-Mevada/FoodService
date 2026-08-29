#include "client/RestaurantClient.h"
#include <curl/curl.h>
#include <cstdlib>

HttpResult RestaurantClient::registerRestaurant(
    const std::string& jsonBody)
{
    return m_httpClient.post(
        "http://localhost:8081/restaurants",
        jsonBody);
}

HttpResult RestaurantClient::getAllRestaurants()
{
    return m_httpClient.get(
        "http://localhost:8081/restaurants");
}

HttpResult RestaurantClient::getRestaurantById(
    int id)
{
    return m_httpClient.get(
        "http://localhost:8081/restaurants/" +
        std::to_string(id));
}

HttpResult RestaurantClient::updateRestaurant(
    int id,
    const std::string& jsonBody)
{
    return m_httpClient.put(
        "http://localhost:8081/restaurants/" +
        std::to_string(id),
        jsonBody);
}

HttpResult RestaurantClient::deleteRestaurant(
    int id)
{
    return m_httpClient.remove(
        "http://localhost:8081/restaurants/" +
        std::to_string(id));
}

HttpResult RestaurantClient::discoverNearby(double latitude, double longitude)
{
    const std::string query = "[out:json][timeout:15];nwr(around:5000," +
        std::to_string(latitude) + "," + std::to_string(longitude) +
        ")[\"amenity\"=\"restaurant\"][\"name\"];out center tags 20;";
    CURL* curl = curl_easy_init();
    if (!curl) return {0, "", TransportFailure::Other, "Could not initialize URL encoder"};
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
