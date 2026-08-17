#pragma once

#include <memory>
#include <mutex>
#include <string>

#include <sqlite3.h>

struct SQLiteCloser
{
    void operator()(sqlite3* db) const
    {
        if (db)
        {
            sqlite3_close(db);
        }
    }
};

class Database
{
public:
    explicit Database(const std::string& databaseName);

    void createRestaurantTable();
    void createOrderTable();
    void createPaymentTable();
    void createNotificationTable();
    void createDriverLocationTable();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    Database(Database&&) = default;
    Database& operator=(Database&&) = default;

    ~Database() = default;

    bool execute(const std::string& sql);

    sqlite3* connection() const
    {
        return m_database.get();
    }

    // A service owns one SQLite connection shared by Crow worker threads.
    // Callers use this lock for multi-call sequences such as INSERT + row id.
    std::recursive_mutex& mutex() const { return m_mutex; }

private:
    std::unique_ptr<sqlite3, SQLiteCloser> m_database;
    mutable std::recursive_mutex m_mutex;
};
