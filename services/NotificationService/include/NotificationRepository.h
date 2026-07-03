#pragma once

#include <optional>
#include <vector>

#include "Database.h"
#include "Notification.h"

class NotificationRepository
{
public:
    explicit NotificationRepository(Database& database);

    bool saveNotification(const Notification& notification);

    std::vector<Notification> getAllNotifications();

    std::optional<Notification> getNotificationById(int id);

    bool updateNotification(const Notification& notification);

    bool deleteNotification(int id);

private:
    Database& m_database;
};
