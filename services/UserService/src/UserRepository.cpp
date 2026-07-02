#include "UserRepository.h"

#include <sqlite3.h>
#include <iostream>

UserRepository::UserRepository(Database& database)
    : m_database(database)
{
}

bool UserRepository::saveUser(const User& user)
{
	std::cout<<" Save User "<<std::endl;
    const char* sql =
        "INSERT INTO users(name,email,password)"
        "VALUES(?,?,?);";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        m_database.connection(),
        sql,
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        std::cerr << sqlite3_errmsg(m_database.connection()) << '\n';
        return false;
    }

    sqlite3_bind_text(stmt, 1, user.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.getEmail().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, user.getPassword().c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        std::cerr << sqlite3_errmsg(m_database.connection()) << '\n';
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
	std::cout<<" Save User "<<std::endl;

    return true;
}

std::vector<User> UserRepository::getAllUsers()
{
    std::vector<User> users;

    const char* sql =
        "SELECT id, name, email, password FROM users;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        m_database.connection(),
        sql,
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        return users;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        int id =
            sqlite3_column_int(stmt, 0);

        std::string name =
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 1));

        std::string email =
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 2));

        std::string password =
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 3));

        users.emplace_back(
            id,
            name,
            email,
            password);
    }

    sqlite3_finalize(stmt);

    return users;
}
