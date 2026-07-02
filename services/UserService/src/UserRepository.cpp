#include "UserRepository.h"

#include <sqlite3.h>
#include <iostream>

UserRepository::UserRepository(Database& database)
    : m_database(database)
{
}

bool UserRepository::saveUser(const User& user)
{
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

    sqlite3_bind_text(stmt, 1, user.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, user.password.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        std::cerr << sqlite3_errmsg(m_database.connection()) << '\n';
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);

    return true;
}
