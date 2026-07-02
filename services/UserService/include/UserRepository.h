#pragma once

#include "Database.h"
#include "User.h"

class UserRepository
{
public:
    explicit UserRepository(Database& database);

    bool saveUser(const User& user);

private:
    Database& m_database;
};
