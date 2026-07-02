#pragma once

#include "Database.h"
#include "Restaurant.h"
#include <vector>
#include <Logger.h>
#include <optional>
class RestaurantRepository
{
public:
    explicit RestaurantRepository(Database& database);

    bool saveRestaurant(const Restaurant& restaurant);

    std::vector<Restaurant> getAllRestaurants();

    std::optional<Restaurant> getRestaurantById(int id);

    bool updateRestaurant(const Restaurant& restaurant);

    bool deleteRestaurant(int id);

private:
    Database& m_database;
};
