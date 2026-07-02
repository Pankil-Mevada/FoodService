#include "UserController.h"
#include "UserService.h"
#include "User.h"
#include <iostream>

crow::response UserController::health()
{
    return crow::response("User Service is Healthy!");
}

crow::response UserController::registerUser(const crow::request& req)
{
	auto json = crow::json::load(req.body);

	if (!json)
	{
		return crow::response(400, "Invalid JSON");
	}

	User user(
			json["name"].s(),
			json["email"].s(),
			json["password"].s()
		 );

	UserService service;

	bool status = service.registerUser(user);

	crow::json::wvalue response;

	if (status)
	{
		response["status"] = "success";
		response["message"] = "User registered";
	}
	else
	{
		response["status"] = "failure";
		response["message"] = "Registration failed";
	}
	return crow::response(response);
}
