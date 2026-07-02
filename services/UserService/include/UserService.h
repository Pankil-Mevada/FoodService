#pragma once
#include <vector>
#include "UserRepository.h"

class UserService
{
public:
    explicit UserService(UserRepository& repository);
	
    bool registerUser(const User& user);
   std::vector<User> getAllUsers();
private:
    UserRepository& m_repository;
};
