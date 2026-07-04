#pragma once
#include <vector>
#include <iostream>
#include "JwtManager.h"
#include <optional>
#include "UserRepository.h"

class UserService
{
public:
    explicit UserService(UserRepository& repository);
	
    bool registerUser(const User& user);
   std::vector<User> getAllUsers();
   std::optional<User> getUserById(int id);
   bool updateUser(const User& user);

bool deleteUser(int id);

std::optional<std::string> login(
    const std::string& email,
    const std::string& password);
private:
    UserRepository& m_repository;
};
