#pragma once

#include <crow.h>

class UserController
{
public:
    static crow::response registerUser(const crow::request& req);

    static crow::response health();
};
