#pragma once

#include "UserRepository.h"

class UserService
{
public:
    explicit UserService(UserRepository& repository);

    bool registerUser(const User& user);

private:
    UserRepository& m_repository;
};
