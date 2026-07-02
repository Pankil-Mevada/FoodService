#pragma once

#include <crow.h>

#include "UserService.h"

class UserController
{
public:
    explicit UserController(UserService& service);

    crow::response registerUser(const crow::request& req);

    crow::response health();
    crow::response getAllUsers();

private:
    UserService& m_service;
};
