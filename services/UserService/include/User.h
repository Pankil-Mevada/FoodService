#pragma once

#include <string>

class User
{
public:
    int id{};
    std::string name;
    std::string email;
    std::string password;

    User() = default;

    User(const std::string& name,
         const std::string& email,
         const std::string& password);
};
