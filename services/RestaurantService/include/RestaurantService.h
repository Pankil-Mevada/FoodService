#pragma once
#include <vector>
#include <optional>
#include "RestaurantRepository.h"

class RestaurantService
{
public:
    explicit RestaurantService(RestaurantRepository& repository);

    bool registerRestaurant(const Restaurant& restaurant);

    std::vector<Restaurant> getAllRestaurants();

    std::optional<Restaurant> getRestaurantById(int id);

    bool updateRestaurant(const Restaurant& restaurant);

    bool deleteRestaurant(int id);

private:
    RestaurantRepository& m_repository;
};
