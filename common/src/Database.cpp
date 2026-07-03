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

void Database::createRestaurantTable()
{
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS restaurants
        (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            address TEXT NOT NULL,
            phone TEXT NOT NULL,
            rating REAL DEFAULT 0.0
        );
    )";

    char* errMsg = nullptr;

    int rc = sqlite3_exec(
        m_database.get(),
        sql,
        nullptr,
        nullptr,
        &errMsg);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to create restaurants table: "
                  << errMsg << std::endl;

        sqlite3_free(errMsg);
    }
}

void Database::createOrderTable()
{
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS orders
        (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            restaurant_id INTEGER NOT NULL,
            total_amount REAL NOT NULL,
            status TEXT NOT NULL
        );
    )";

    char* errMsg = nullptr;

    int rc = sqlite3_exec(
        m_database.get(),
        sql,
        nullptr,
        nullptr,
        &errMsg);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to create orders table: "
                  << errMsg << std::endl;

        sqlite3_free(errMsg);
    }
}

void Database::createPaymentTable()
{
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS payments
        (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_id INTEGER NOT NULL,
            amount REAL NOT NULL,
            payment_method TEXT NOT NULL,
            transaction_id TEXT NOT NULL,
            status TEXT NOT NULL
        );
    )";

    char* errMsg = nullptr;

    int rc = sqlite3_exec(
        connection(),
        sql,
        nullptr,
        nullptr,
        &errMsg);

    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to create payments table: "
                  << errMsg << std::endl;

        sqlite3_free(errMsg);
    }
}
void Database::createNotificationTable()
{
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS notifications
        (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            type TEXT NOT NULL,
            message TEXT NOT NULL,
            status TEXT NOT NULL,
            created_at TEXT NOT NULL
        );
    )";

    char* errMsg = nullptr;

    int rc = sqlite3_exec(
        connection(),
        sql,
        nullptr,
        nullptr,
        &errMsg);

    if (rc != SQLITE_OK)
    {
        std::cerr
            << "Failed to create notifications table: "
            << errMsg
            << std::endl;

        sqlite3_free(errMsg);
    }
}
