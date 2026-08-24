#include "RestaurantController.h"
#include "RestaurantService.h"
#include "Restaurant.h"
#include "UserValidator.h"
#include <iostream>

RestaurantController::RestaurantController(RestaurantService& service)
    : m_service(service)
{
}
crow::response RestaurantController::health()
{
    return crow::response("Restaurant Service is Healthy!");
}

crow::response RestaurantController::registerRestaurant(const crow::request& req)
{
	auto json = crow::json::load(req.body);

	if (!json)
	{
		return crow::response(400, "Invalid JSON");
	}

	Restaurant restaurant(
			0,
			json["name"].s(),
			json["address"].s(),
			json["phone"].s(),
			json["rating"].d(),
			json.has("latitude") ? json["latitude"].d() : 23.0225,
			json.has("longitude") ? json["longitude"].d() : 72.5714,
			json.has("deliveryRadiusKm") ? json["deliveryRadiusKm"].d() : 8.0,
			json.has("imageUrl") ? std::string(json["imageUrl"].s()) : "",
            json.has("deliveryPolygon") ? std::string(json["deliveryPolygon"].s()) : "",
            json.has("baseDeliveryFee") ? json["baseDeliveryFee"].d() : 39.0,
            json.has("perKmFee") ? json["perKmFee"].d() : 5.0,
            json.has("preparationMinutes") ? json["preparationMinutes"].i() : 20
			);
	if (!UserValidator::validateName(restaurant.getName()))
	{
		return crow::response(400, "Invalid name");
	}

	bool status = m_service.registerRestaurant(restaurant);

	crow::json::wvalue response;

	if (status)
	{
		response["success"] = true;
		response["message"] = "Restaurant registered successfully";

		return crow::response(201, response);
	}
	else
	{
		response["success"] = false;
		response["message"] = " Resturant already exists";

		return crow::response(409, response);
	}
	return crow::response(response);
}

crow::response RestaurantController::getAllRestaurants()
{
    auto restaurants  = m_service.getAllRestaurants();

    crow::json::wvalue response;

    std::size_t index = 0;

    for (const auto& restaurant : restaurants)
    {
	    response[index]["id"] = restaurant.getId();
	    response[index]["name"] = restaurant.getName();
	    response[index]["address"] = restaurant.getAddress();
	    response[index]["phone"] = restaurant.getPhone();
	    response[index]["rating"] = restaurant.getRating();
	    response[index]["latitude"] = restaurant.getLatitude();
	    response[index]["longitude"] = restaurant.getLongitude();
	    response[index]["deliveryRadiusKm"] = restaurant.getDeliveryRadiusKm();
	    response[index]["imageUrl"] = restaurant.getImageUrl();
	    response[index]["deliveryPolygon"] = restaurant.getDeliveryPolygon(); response[index]["baseDeliveryFee"] = restaurant.getBaseDeliveryFee(); response[index]["perKmFee"] = restaurant.getPerKmFee(); response[index]["preparationMinutes"] = restaurant.getPreparationMinutes();

	    ++index;
    }
    return crow::response(response);
}
crow::response RestaurantController::getRestaurantById(int id)
{
    auto restaurant = m_service.getRestaurantById(id);

    if(!restaurant.has_value())
    {
        crow::json::wvalue response;

        response["success"] = false;
        response["message"] = "Restaurant not found";

        return crow::response(404,response);
    }

    crow::json::wvalue response;

    response["id"] = restaurant->getId();
    response["name"] = restaurant->getName();
    response["address"] = restaurant->getAddress();
    response["phone"] = restaurant->getPhone();
    response["rating"] = restaurant->getRating();
    response["latitude"] = restaurant->getLatitude();
    response["longitude"] = restaurant->getLongitude();
    response["deliveryRadiusKm"] = restaurant->getDeliveryRadiusKm();
    response["imageUrl"] = restaurant->getImageUrl();
    response["deliveryPolygon"] = restaurant->getDeliveryPolygon(); response["baseDeliveryFee"] = restaurant->getBaseDeliveryFee(); response["perKmFee"] = restaurant->getPerKmFee(); response["preparationMinutes"] = restaurant->getPreparationMinutes();
    
    return crow::response(response);
}

crow::response RestaurantController::updateRestaurant(
    int id,
    const crow::request& req)
{
    auto json = crow::json::load(req.body);

    if (!json)
    {
        return crow::response(400, "Invalid JSON");
    }

    Restaurant restaurant(
        id,
        json["name"].s(),
        json["address"].s(),
        json["phone"].s(),
        json["rating"].d(),
        json.has("latitude") ? json["latitude"].d() : 23.0225,
        json.has("longitude") ? json["longitude"].d() : 72.5714,
        json.has("deliveryRadiusKm") ? json["deliveryRadiusKm"].d() : 8.0,
        json.has("imageUrl") ? std::string(json["imageUrl"].s()) : "", json.has("deliveryPolygon") ? std::string(json["deliveryPolygon"].s()) : "", json.has("baseDeliveryFee") ? json["baseDeliveryFee"].d() : 39.0, json.has("perKmFee") ? json["perKmFee"].d() : 5.0, json.has("preparationMinutes") ? json["preparationMinutes"].i() : 20);
	
    if (!UserValidator::validateName(restaurant.getName()))
    {
	    return crow::response(400, "Invalid name");
    }

    bool status = m_service.updateRestaurant(restaurant);

    crow::json::wvalue response;

    if (status)
    {
        response["success"] = true;
        response["message"] = "Restaurant updated successfully";
    }
    else
    {
        response["success"] = false;
        response["message"] = "Restaurant not found";
    }

    return crow::response(response);
}

crow::response RestaurantController::deleteRestaurant(int id)
{
    bool status = m_service.deleteRestaurant(id);

    crow::json::wvalue response;

    if (status)
    {
        response["success"] = true;
        response["message"] = "Restaurant deleted successfully";
    }
    else
    {
        response["success"] = false;
        response["message"] = "Restaurant not found";
    }

    return crow::response(response);
}
