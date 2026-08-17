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

    // Wait for short write bursts rather than failing immediately with
    // SQLITE_BUSY. WAL permits readers while the single SQLite writer drains.
    sqlite3_busy_timeout(db, 30000);
    execute("PRAGMA journal_mode=WAL;");
    execute("PRAGMA synchronous=NORMAL;");
    execute("PRAGMA foreign_keys=ON;");

    std::cout << "Database opened successfully\n";
}

bool Database::execute(const std::string& sql)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
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
            rating REAL DEFAULT 0.0,
            latitude REAL NOT NULL DEFAULT 23.0225,
            longitude REAL NOT NULL DEFAULT 72.5714,
            delivery_radius_km REAL NOT NULL DEFAULT 8.0,
            image_url TEXT NOT NULL DEFAULT ''
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
    sqlite3_exec(connection(), "ALTER TABLE restaurants ADD COLUMN latitude REAL NOT NULL DEFAULT 23.0225;", nullptr, nullptr, nullptr);
    sqlite3_exec(connection(), "ALTER TABLE restaurants ADD COLUMN longitude REAL NOT NULL DEFAULT 72.5714;", nullptr, nullptr, nullptr);
    sqlite3_exec(connection(), "ALTER TABLE restaurants ADD COLUMN delivery_radius_km REAL NOT NULL DEFAULT 8.0;", nullptr, nullptr, nullptr);
    sqlite3_exec(connection(), "ALTER TABLE restaurants ADD COLUMN image_url TEXT NOT NULL DEFAULT '';", nullptr, nullptr, nullptr);
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
            status TEXT NOT NULL,
            delivery_latitude REAL NOT NULL DEFAULT 0,
            delivery_longitude REAL NOT NULL DEFAULT 0,
            delivery_address TEXT NOT NULL DEFAULT '',
            item_summary TEXT NOT NULL DEFAULT '',
            subtotal REAL NOT NULL DEFAULT 0,
            discount_amount REAL NOT NULL DEFAULT 0,
            delivery_fee REAL NOT NULL DEFAULT 0
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
    sqlite3_exec(connection(), "ALTER TABLE orders ADD COLUMN delivery_latitude REAL NOT NULL DEFAULT 0;", nullptr, nullptr, nullptr);
    sqlite3_exec(connection(), "ALTER TABLE orders ADD COLUMN delivery_longitude REAL NOT NULL DEFAULT 0;", nullptr, nullptr, nullptr);
    sqlite3_exec(connection(), "ALTER TABLE orders ADD COLUMN delivery_address TEXT NOT NULL DEFAULT '';", nullptr, nullptr, nullptr);
    sqlite3_exec(connection(), "ALTER TABLE orders ADD COLUMN item_summary TEXT NOT NULL DEFAULT '';", nullptr, nullptr, nullptr);
    sqlite3_exec(connection(), "ALTER TABLE orders ADD COLUMN subtotal REAL NOT NULL DEFAULT 0;", nullptr, nullptr, nullptr);
    sqlite3_exec(connection(), "ALTER TABLE orders ADD COLUMN discount_amount REAL NOT NULL DEFAULT 0;", nullptr, nullptr, nullptr);
    sqlite3_exec(connection(), "ALTER TABLE orders ADD COLUMN delivery_fee REAL NOT NULL DEFAULT 0;", nullptr, nullptr, nullptr);
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
            transaction_id TEXT NOT NULL UNIQUE,
            status TEXT NOT NULL,
            idempotency_key TEXT UNIQUE,
            provider TEXT NOT NULL DEFAULT 'test',
            provider_payment_id TEXT,
            updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
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

    // Backwards-compatible migrations for databases created by older builds.
    // SQLite reports duplicate-column errors on subsequent starts; those are safe.
    sqlite3_exec(connection(), "ALTER TABLE payments ADD COLUMN idempotency_key TEXT;", nullptr, nullptr, nullptr);
    sqlite3_exec(connection(), "ALTER TABLE payments ADD COLUMN provider TEXT NOT NULL DEFAULT 'test';", nullptr, nullptr, nullptr);
    sqlite3_exec(connection(), "ALTER TABLE payments ADD COLUMN provider_payment_id TEXT;", nullptr, nullptr, nullptr);
    sqlite3_exec(connection(), "ALTER TABLE payments ADD COLUMN updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP;", nullptr, nullptr, nullptr);
    sqlite3_exec(connection(), "CREATE UNIQUE INDEX IF NOT EXISTS idx_payments_idempotency_key ON payments(idempotency_key) WHERE idempotency_key IS NOT NULL;", nullptr, nullptr, nullptr);
    sqlite3_exec(connection(), "CREATE UNIQUE INDEX IF NOT EXISTS idx_payments_transaction_id ON payments(transaction_id);", nullptr, nullptr, nullptr);
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

void Database::createDriverLocationTable()
{
    execute(R"(
        CREATE TABLE IF NOT EXISTS driver_locations
        (
            order_id INTEGER PRIMARY KEY,
            latitude REAL NOT NULL,
            longitude REAL NOT NULL,
            accuracy_m REAL NOT NULL DEFAULT 0,
            speed_mps REAL NOT NULL DEFAULT 0,
            heading REAL NOT NULL DEFAULT 0,
            delivery_status TEXT NOT NULL,
            driver_name TEXT NOT NULL,
            driver_contact TEXT NOT NULL,
            vehicle_type TEXT NOT NULL,
            vehicle_plate TEXT NOT NULL,
            updated_epoch INTEGER NOT NULL
        );
    )");
}
