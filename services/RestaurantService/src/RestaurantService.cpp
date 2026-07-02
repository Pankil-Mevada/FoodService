#include "RestaurantService.h"

RestaurantService::RestaurantService(RestaurantRepository& repository)
    : m_repository(repository)
{
}

bool RestaurantService::registerRestaurant(const Restaurant& restaurant)
{
    return m_repository.saveRestaurant(restaurant);
}

std::vector<Restaurant> RestaurantService::getAllRestaurants()
{
    return m_repository.getAllRestaurants();
}

std::optional<Restaurant> RestaurantService::getRestaurantById(int id)
{
    return m_repository.getRestaurantById(id);
}

bool RestaurantService::updateRestaurant(const Restaurant& restaurant)
{
    return m_repository.updateRestaurant(restaurant);
}

bool RestaurantService::deleteRestaurant(int id)
{
    return m_repository.deleteRestaurant(id);
}
