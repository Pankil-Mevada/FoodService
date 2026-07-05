#pragma once

#include <string>
#include "client/HttpClient.h"
class RestaurantClient
{
public:

    std::string registerRestaurant(
        const std::string& jsonBody);

    std::string getAllRestaurants();

    std::string getRestaurantById(
        int id);

    std::string updateRestaurant(
        int id,
        const std::string& jsonBody);

    std::string deleteRestaurant(
        int id);

private:

    HttpClient m_httpClient;
};