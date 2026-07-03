#pragma once

#include <crow.h>

#include "NotificationService.h"

class NotificationController
{
public:

    explicit NotificationController(NotificationService& service);

    crow::response health();

    crow::response createNotification(const crow::request& req);

    crow::response getAllNotifications();

    crow::response getNotificationById(int id);

    crow::response updateNotification(
        int id,
        const crow::request& req);

    crow::response deleteNotification(int id);

private:

    NotificationService& m_service;
};
