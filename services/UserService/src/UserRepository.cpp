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
std::optional<User> UserRepository::getUserById(int id)
{
    const char* sql =
        "SELECT id,name,email,password "
        "FROM users "
        "WHERE id=?;";

    sqlite3_stmt* stmt = nullptr;

    int rc =
        sqlite3_prepare_v2(
            m_database.connection(),
            sql,
            -1,
            &stmt,
            nullptr);

    if(rc != SQLITE_OK)
    {
        return std::nullopt;
    }

    sqlite3_bind_int(stmt,1,id);

    rc = sqlite3_step(stmt);

    if(rc == SQLITE_ROW)
    {
        User user(
            sqlite3_column_int(stmt,0),

            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt,1)),

            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt,2)),

            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt,3)));

        sqlite3_finalize(stmt);

        return user;
    }

    sqlite3_finalize(stmt);

    return std::nullopt;
}

bool UserRepository::updateUser(const User& user)
{
    const char* sql =
        "UPDATE users "
        "SET name = ?, email = ?, password = ? "
        "WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        m_database.connection(),
        sql,
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, user.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.getEmail().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, user.getPassword().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, user.getId());

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        return false;
    }

    return sqlite3_changes(m_database.connection()) > 0;
}


bool UserRepository::deleteUser(int id)
{
    const char* sql =
        "DELETE FROM users WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        m_database.connection(),
        sql,
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        return false;
    }

    return sqlite3_changes(m_database.connection()) > 0;
}


std::optional<User> UserRepository::findByEmail(
    const std::string& email)
{
    const char* sql =
        "SELECT id,name,email,password "
        "FROM users "
        "WHERE email=?;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        m_database.connection(),
        sql,
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        return std::nullopt;
    }

    sqlite3_bind_text(
        stmt,
        1,
        email.c_str(),
        -1,
        SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        User user(
            sqlite3_column_int(stmt,0),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt,1)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt,2)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt,3))
        );

        sqlite3_finalize(stmt);

        return user;
    }

    sqlite3_finalize(stmt);

    return std::nullopt;
}
