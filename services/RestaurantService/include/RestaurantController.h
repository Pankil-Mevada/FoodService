#pragma once

#include <crow.h>

#include "RestaurantService.h"

class RestaurantController
{
public:
    explicit RestaurantController(RestaurantService& service);

    crow::response registerRestaurant(const crow::request& req);

    crow::response health();
    crow::response getAllRestaurants();
    crow::response getRestaurantById(int id);
    crow::response updateRestaurant(
    int id,
    const crow::request& req);

crow::response deleteRestaurant(int id);

crow::response login(
    const crow::request& req);

private:
    RestaurantService& m_service;
};
