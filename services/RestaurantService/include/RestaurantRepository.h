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

    bool saveRestaurant(const Restaurant& user);
    std::vector<Restaurant> getAllRestaurants();
	std::optional<Restaurant> getRestaurantById(int id);
	bool updateRestaurant(const Restaurant& user);

bool deleteRestaurant(int id);

std::optional<Restaurant> findByEmail(
    const std::string& email);
private:
    Database& m_database;
};
