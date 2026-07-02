#pragma once

#include "Database.h"
#include "User.h"
#include <vector>
#include <Logger.h>
#include <optional>
class UserRepository
{
public:
    explicit UserRepository(Database& database);

    bool saveUser(const User& user);
    std::vector<User> getAllUsers();
	std::optional<User> getUserById(int id);
	bool updateUser(const User& user);

bool deleteUser(int id);

std::optional<User> findByEmail(
    const std::string& email);
private:
    Database& m_database;
};
