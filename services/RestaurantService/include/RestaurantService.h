#pragma once
#include <vector>
#include <optional>
#include "RestaurantRepository.h"

class RestaurantService
{
public:
    explicit RestaurantService(RestaurantRepository& repository);
	
    bool registerRestaurant(const Restaurant& user);
   std::vector<Restaurant> getAllRestaurants();
   std::optional<Restaurant> getRestaurantById(int id);
   bool updateRestaurant(const Restaurant& user);

bool deleteRestaurant(int id);

bool login(
    const std::string& email,
    const std::string& password);
private:
    RestaurantRepository& m_repository;
};
