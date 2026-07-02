#include "Database.h"

#include <iostream>
#include <stdexcept>

Database::Database(const std::string& databaseName)
{
    sqlite3* db = nullptr;

    int rc = sqlite3_open(databaseName.c_str(), &db);

    if (rc != SQLITE_OK)
    {
        std::string error =
            db ? sqlite3_errmsg(db) : "Unknown database error";

        if (db)
        {
            sqlite3_close(db);
        }

        throw std::runtime_error(error);
    }

    m_database.reset(db);

    std::cout << "Database opened successfully\n";
}

bool Database::execute(const std::string& sql)
{
    char* errorMessage = nullptr;

    int rc = sqlite3_exec(
        m_database.get(),
        sql.c_str(),
        nullptr,
        nullptr,
        &errorMessage);

    if (rc != SQLITE_OK)
    {
        std::cerr << "SQLite Error : "
                  << errorMessage
                  << std::endl;

        sqlite3_free(errorMessage);

        return false;
    }

    return true;
}
