#pragma once

#include <string>

class UserValidator
{
public:
    static bool validateName(const std::string& name);

    static bool validateEmail(const std::string& email);

    static bool validatePassword(const std::string& password);
};
