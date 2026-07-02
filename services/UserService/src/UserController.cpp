#include "UserController.h"

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

    std::string name = json["name"].s();
    std::string email = json["email"].s();
    std::string password = json["password"].s();

    std::cout << "Name     : " << name << std::endl;
    std::cout << "Email    : " << email << std::endl;
    std::cout << "Password : " << password << std::endl;

    crow::json::wvalue response;

    response["status"] = "success";
    response["message"] = "User registered";

    return crow::response(response);
}
