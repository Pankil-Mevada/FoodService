#pragma once

#include <memory>
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

private:
    std::unique_ptr<sqlite3, SQLiteCloser> m_database;
};
