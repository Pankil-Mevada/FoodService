#include "OrderRepository.h"

#include <iostream>
#include <sqlite3.h>

OrderRepository::OrderRepository(Database& database)
    : m_database(database)
{
}

bool OrderRepository::saveOrder(const Order& order)
{
    const char* sql =
        "INSERT INTO orders(user_id,restaurant_id,total_amount,status)"
        "VALUES(?,?,?,?);";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        m_database.connection(),
        sql,
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        std::cerr << sqlite3_errmsg(m_database.connection()) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, order.getUserId());
    sqlite3_bind_int(stmt, 2, order.getRestaurantId());
    sqlite3_bind_double(stmt, 3, order.getTotalAmount());
    sqlite3_bind_text(stmt,
                      4,
                      order.getStatus().c_str(),
                      -1,
                      SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::vector<Order> OrderRepository::getAllOrders()
{
    std::vector<Order> orders;

    const char* sql =
        "SELECT id,user_id,restaurant_id,total_amount,status "
        "FROM orders;";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        m_database.connection(),
        sql,
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        return orders;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        orders.emplace_back(
            sqlite3_column_int(stmt,0),
            sqlite3_column_int(stmt,1),
            sqlite3_column_int(stmt,2),
            sqlite3_column_double(stmt,3),
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt,4)));
    }

    sqlite3_finalize(stmt);

    return orders;
}

std::optional<Order> OrderRepository::getOrderById(int id)
{
    const char* sql =
        "SELECT id,user_id,restaurant_id,total_amount,status "
        "FROM orders WHERE id=?;";

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

    sqlite3_bind_int(stmt,1,id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Order order(
            sqlite3_column_int(stmt,0),
            sqlite3_column_int(stmt,1),
            sqlite3_column_int(stmt,2),
            sqlite3_column_double(stmt,3),
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt,4)));

        sqlite3_finalize(stmt);

        return order;
    }

    sqlite3_finalize(stmt);

    return std::nullopt;
}

bool OrderRepository::updateOrder(const Order& order)
{
    const char* sql =
        "UPDATE orders "
        "SET user_id=?,restaurant_id=?,total_amount=?,status=? "
        "WHERE id=?;";

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

    sqlite3_bind_int(stmt,1,order.getUserId());
    sqlite3_bind_int(stmt,2,order.getRestaurantId());
    sqlite3_bind_double(stmt,3,order.getTotalAmount());
    sqlite3_bind_text(stmt,
                      4,
                      order.getStatus().c_str(),
                      -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,5,order.getId());

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        return false;
    }

    return sqlite3_changes(m_database.connection()) > 0;
}

bool OrderRepository::deleteOrder(int id)
{
    const char* sql =
        "DELETE FROM orders WHERE id=?;";

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

    sqlite3_bind_int(stmt,1,id);

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        return false;
    }

    return sqlite3_changes(m_database.connection()) > 0;
}
