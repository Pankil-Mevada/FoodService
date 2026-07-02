#include "RestaurantService.h"

RestaurantService::RestaurantService(RestaurantRepository& repository)
    : m_repository(repository)
{
}

bool RestaurantService::registerRestaurant(const Restaurant& user)
{
	auto existingRestaurant = m_repository.findByEmail(user.getEmail());

	if (existingRestaurant.has_value())
	{
		return false;
	}
    return m_repository.saveRestaurant(user);
}
std::vector<Restaurant> RestaurantService::getAllRestaurants()
{
    return m_repository.getAllRestaurants();
}

std::optional<Restaurant> RestaurantService::getRestaurantById(int id)
{
    return m_repository.getRestaurantById(id);
}

bool RestaurantService::updateRestaurant(const Restaurant& user)
{
    return m_repository.updateRestaurant(user);
}

bool RestaurantService::deleteRestaurant(int id)
{
    return m_repository.deleteRestaurant(id);
}

bool RestaurantService::login(
    const std::string& email,
    const std::string& password)
{
    auto user = m_repository.findByEmail(email);

    if (!user.has_value())
    {
        return false;
    }

    if (user->getPassword() != password)
    {
        return false;
    }

    return true;
}
