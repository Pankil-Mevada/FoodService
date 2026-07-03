#include "NotificationRepository.h"

#include <iostream>
#include <sqlite3.h>

NotificationRepository::NotificationRepository(Database& database)
    : m_database(database)
{
}

bool NotificationRepository::saveNotification(
    const Notification& notification)
{
    const char* sql =
        "INSERT INTO notifications"
        "(user_id,type,message,status,created_at)"
        "VALUES(?,?,?,?,?);";

    sqlite3_stmt* stmt = nullptr;

    int rc =
        sqlite3_prepare_v2(
            m_database.connection(),
            sql,
            -1,
            &stmt,
            nullptr);

    if (rc != SQLITE_OK)
    {
        std::cerr << sqlite3_errmsg(
            m_database.connection()) << '\n';

        return false;
    }

    sqlite3_bind_int(
        stmt,
        1,
        notification.getUserId());

    sqlite3_bind_text(
        stmt,
        2,
        notification.getType().c_str(),
        -1,
        SQLITE_TRANSIENT);

    sqlite3_bind_text(
        stmt,
        3,
        notification.getMessage().c_str(),
        -1,
        SQLITE_TRANSIENT);

    sqlite3_bind_text(
        stmt,
        4,
        notification.getStatus().c_str(),
        -1,
        SQLITE_TRANSIENT);

    sqlite3_bind_text(
        stmt,
        5,
        notification.getCreatedAt().c_str(),
        -1,
        SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::vector<Notification>
NotificationRepository::getAllNotifications()
{
    std::vector<Notification> notifications;

    const char* sql =
        "SELECT id,user_id,type,message,status,created_at "
        "FROM notifications;";

    sqlite3_stmt* stmt = nullptr;

    int rc =
        sqlite3_prepare_v2(
            m_database.connection(),
            sql,
            -1,
            &stmt,
            nullptr);

    if (rc != SQLITE_OK)
    {
        return notifications;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        notifications.emplace_back(
            sqlite3_column_int(stmt,0),
            sqlite3_column_int(stmt,1),
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt,2)),
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt,3)),
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt,4)),
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt,5)));
    }

    sqlite3_finalize(stmt);

    return notifications;
}

std::optional<Notification>
NotificationRepository::getNotificationById(int id)
{
    const char* sql =
        "SELECT id,user_id,type,message,status,created_at "
        "FROM notifications "
        "WHERE id=?;";

    sqlite3_stmt* stmt = nullptr;

    int rc =
        sqlite3_prepare_v2(
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
        Notification notification(
            sqlite3_column_int(stmt,0),
            sqlite3_column_int(stmt,1),
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt,2)),
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt,3)),
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt,4)),
            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt,5)));

        sqlite3_finalize(stmt);

        return notification;
    }

    sqlite3_finalize(stmt);

    return std::nullopt;
}
bool NotificationRepository::updateNotification(
    const Notification& notification)
{
    const char* sql =
        "UPDATE notifications "
        "SET user_id=?,type=?,message=?,status=?,created_at=? "
        "WHERE id=?;";

    sqlite3_stmt* stmt = nullptr;

    int rc =
        sqlite3_prepare_v2(
            m_database.connection(),
            sql,
            -1,
            &stmt,
            nullptr);

    if (rc != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(stmt,1,notification.getUserId());

    sqlite3_bind_text(stmt,2,
        notification.getType().c_str(),
        -1,SQLITE_TRANSIENT);

    sqlite3_bind_text(stmt,3,
        notification.getMessage().c_str(),
        -1,SQLITE_TRANSIENT);

    sqlite3_bind_text(stmt,4,
        notification.getStatus().c_str(),
        -1,SQLITE_TRANSIENT);

    sqlite3_bind_text(stmt,5,
        notification.getCreatedAt().c_str(),
        -1,SQLITE_TRANSIENT);

    sqlite3_bind_int(stmt,6,notification.getId());

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if(rc != SQLITE_DONE)
    {
        return false;
    }

    return sqlite3_changes(
        m_database.connection()) > 0;
}

bool NotificationRepository::deleteNotification(int id)
{
    const char* sql =
        "DELETE FROM notifications WHERE id=?;";

    sqlite3_stmt* stmt = nullptr;

    int rc =
        sqlite3_prepare_v2(
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

    if(rc != SQLITE_DONE)
    {
        return false;
    }

    return sqlite3_changes(
        m_database.connection()) > 0;
}
