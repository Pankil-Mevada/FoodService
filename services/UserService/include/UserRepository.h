#pragma once

#include "Database.h"
#include "User.h"
#include <vector>

class UserRepository
{
public:
    explicit UserRepository(Database& database);

    bool saveUser(const User& user);
    std::vector<User> getAllUsers();

private:
    Database& m_database;
};
