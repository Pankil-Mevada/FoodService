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

    HttpResult partnerGet(const std::string& path, const std::string& authHeader);
    HttpResult partnerPost(const std::string& path, const std::string& jsonBody,
        const std::string& authHeader);
    HttpResult partnerPut(const std::string& path, const std::string& jsonBody,
        const std::string& authHeader);
    HttpResult partnerDelete(const std::string& path, const std::string& authHeader);

private:

    HttpClient m_httpClient;
};
