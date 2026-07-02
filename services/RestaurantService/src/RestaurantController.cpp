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

	Restaurant user(
			0,
			json["name"].s(),
			json["email"].s(),
			json["password"].s()
		 );

	if (!UserValidator::validateName(user.getName()))
	{
		return crow::response(400, "Invalid name");
	}

	if (!UserValidator::validateEmail(user.getAddress()))
	{
		return crow::response(400, "Invalid address");
	}

	if (!UserValidator::validatePassword(user.getPhone()))
	{
		return crow::response(400, "Phone must be at least 6 characters");
	}

	bool status = m_service.registerRestaurant(user);

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
    auto users = m_service.getAllRestaurants();

    crow::json::wvalue response;

    std::size_t index = 0;

    for (const auto& user : users)
    {
        response[index]["id"] = user.getId();
        response[index]["address"] = user.getAddress();
        response[index]["phone"] = user.getPhone();
        response[index]["rating"] = user.getRating();

        ++index;
    }

    return crow::response(response);
}
crow::response RestaurantController::getRestaurantById(int id)
{
    auto user = m_service.getRestaurantById(id);

    if(!user.has_value())
    {
        crow::json::wvalue response;

        response["success"] = false;
        response["message"] = "Restaurant not found";

        return crow::response(404,response);
    }

    crow::json::wvalue response;

    response["id"] = user->getId();
    response["address"] = user->getAddress();
    response["phone"] = user->getPhone();
    response["rating"] = user->getRating();

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

    Restaurant user(
        id,
        json["name"].s(),
        json["address"].s(),
        json["phone"].s(),
        json["rating"].s());
	
    if (!UserValidator::validateName(user.getName()))
    {
	    return crow::response(400, "Invalid name");
    }

    if (!UserValidator::validateEmail(user.getAddress()))
    {
	    return crow::response(400, "Invalid email");
    }

    if (!UserValidator::validatePassword(user.getPhone()))
    {
	    return crow::response(400, "Password must be at least 6 characters");
    }
    bool status = m_service.updateRestaurant(user);

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

crow::response RestaurantController::login(
    const crow::request& req)
{
    auto json = crow::json::load(req.body);

    if (!json)
    {
        return crow::response(400, "Invalid JSON");
    }

    bool status =
        m_service.login(
            json["email"].s(),
            json["password"].s());

    crow::json::wvalue response;

    if (status)
    {
        response["success"] = true;
        response["message"] = "Login successful";
    }
    else
    {
        response["success"] = false;
        response["message"] = "Invalid email or password";
    }

    return crow::response(response);
}
