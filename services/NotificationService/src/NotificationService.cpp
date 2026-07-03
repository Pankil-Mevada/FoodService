#include "NotificationService.h"

#include <ctime>
#include <iomanip>
#include <sstream>

NotificationService::NotificationService(
    NotificationRepository& repository)
    : m_repository(repository)
{
}

bool NotificationService::createNotification(
    int userId,
    const std::string& type,
    const std::string& message)
{
    Notification notification(
        0,
        userId,
        type,
        message,
        "SENT",
        generateTimestamp());

    return m_repository.saveNotification(notification);
}

std::vector<Notification>
NotificationService::getAllNotifications()
{
    return m_repository.getAllNotifications();
}

std::optional<Notification>
NotificationService::getNotificationById(int id)
{
    return m_repository.getNotificationById(id);
}

bool NotificationService::updateNotification(
    const Notification& notification)
{
    return m_repository.updateNotification(notification);
}

bool NotificationService::deleteNotification(int id)
{
    return m_repository.deleteNotification(id);
}

std::string NotificationService::generateTimestamp()
{
    std::time_t now = std::time(nullptr);

    std::tm* tm = std::localtime(&now);

    std::ostringstream oss;

    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");

    return oss.str();
}
