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
    crow::response getUserById(int id);
    crow::response updateUser(
    int id,
    const crow::request& req);

crow::response deleteUser(int id);

crow::response login(
    const crow::request& req);

private:
    UserService& m_service;
};
