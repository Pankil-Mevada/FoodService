#include "UserController.h"
#include "UserService.h"
#include "User.h"
#include "UserValidator.h"
#include <iostream>

UserController::UserController(UserService& service)
    : m_service(service)
{
}
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
			0,
			json["name"].s(),
			json["email"].s(),
			json["password"].s()
		 );

	if (!UserValidator::validateName(user.getName()))
	{
		return crow::response(400, "Invalid name");
	}

	if (!UserValidator::validateEmail(user.getEmail()))
	{
		return crow::response(400, "Invalid email");
	}

	if (!UserValidator::validatePassword(user.getPassword()))
	{
		return crow::response(400, "Password must be at least 6 characters");
	}

	bool status = m_service.registerUser(user);

	crow::json::wvalue response;

	if (status)
	{
		response["success"] = true;
		response["message"] = "User registered successfully";

		return crow::response(201, response);
	}
	else
	{
		response["success"] = false;
		response["message"] = "Email already exists";

		return crow::response(409, response);
	}
	return crow::response(response);
}

crow::response UserController::getAllUsers()
{
    auto users = m_service.getAllUsers();

    crow::json::wvalue response;

    std::size_t index = 0;

    for (const auto& user : users)
    {
        response[index]["id"] = user.getId();
        response[index]["name"] = user.getName();
        response[index]["email"] = user.getEmail();

        ++index;
    }

    return crow::response(response);
}
crow::response UserController::getUserById(int id)
{
    auto user = m_service.getUserById(id);

    if(!user.has_value())
    {
        crow::json::wvalue response;

        response["success"] = false;
        response["message"] = "User not found";

        return crow::response(404,response);
    }

    crow::json::wvalue response;

    response["id"] = user->getId();
    response["name"] = user->getName();
    response["email"] = user->getEmail();

    return crow::response(response);
}

crow::response UserController::updateUser(
    int id,
    const crow::request& req)
{
    auto json = crow::json::load(req.body);

    if (!json)
    {
        return crow::response(400, "Invalid JSON");
    }

    User user(
        id,
        json["name"].s(),
        json["email"].s(),
        json["password"].s());
	
    if (!UserValidator::validateName(user.getName()))
    {
	    return crow::response(400, "Invalid name");
    }

    if (!UserValidator::validateEmail(user.getEmail()))
    {
	    return crow::response(400, "Invalid email");
    }

    if (!UserValidator::validatePassword(user.getPassword()))
    {
	    return crow::response(400, "Password must be at least 6 characters");
    }
    bool status = m_service.updateUser(user);

    crow::json::wvalue response;

    if (status)
    {
        response["success"] = true;
        response["message"] = "User updated successfully";
    }
    else
    {
        response["success"] = false;
        response["message"] = "User not found";
    }

    return crow::response(response);
}

crow::response UserController::deleteUser(int id)
{
    bool status = m_service.deleteUser(id);

    crow::json::wvalue response;

    if (status)
    {
        response["success"] = true;
        response["message"] = "User deleted successfully";
    }
    else
    {
        response["success"] = false;
        response["message"] = "User not found";
    }

    return crow::response(response);
}

crow::response UserController::login(
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
