#pragma once

#include <string>
#include "client/HttpClient.h"
class RestaurantClient
{
public:

    HttpResult registerRestaurant(
        const std::string& jsonBody);

    HttpResult getAllRestaurants();

    HttpResult getRestaurantById(
        int id);

    HttpResult updateRestaurant(
        int id,
        const std::string& jsonBody);

    HttpResult deleteRestaurant(
        int id);

    HttpResult discoverNearby(
        double latitude,
        double longitude);

private:

    HttpClient m_httpClient;
};
