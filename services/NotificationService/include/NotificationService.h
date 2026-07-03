#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Notification.h"
#include "NotificationRepository.h"

class NotificationService
{
public:

    explicit NotificationService(NotificationRepository& repository);

    bool createNotification(
        int userId,
        const std::string& type,
        const std::string& message);

    std::vector<Notification> getAllNotifications();

    std::optional<Notification> getNotificationById(int id);

    bool updateNotification(const Notification& notification);

    bool deleteNotification(int id);

private:

    std::string generateTimestamp();

    NotificationRepository& m_repository;
};
