#include "UserService.h"

#include <iostream>

bool UserService::registerUser(const User& user)
{
    std::cout << "\n===== User Registration =====\n";
    std::cout << "Name     : " << user.name << '\n';
    std::cout << "Email    : " << user.email << '\n';
    std::cout << "Password : " << user.password << '\n';
    std::cout << "=============================\n";

    return true;
}
