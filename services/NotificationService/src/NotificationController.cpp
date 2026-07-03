#include "NotificationController.h"

NotificationController::NotificationController(
    NotificationService& service)
    : m_service(service)
{
}

crow::response NotificationController::health()
{
    return crow::response("Notification Service is Healthy!");
}

crow::response NotificationController::createNotification(
    const crow::request& req)
{
    auto json = crow::json::load(req.body);

    if (!json)
    {
        return crow::response(400, "Invalid JSON");
    }

    bool status =
        m_service.createNotification(
            json["userId"].i(),
            json["type"].s(),
            json["message"].s());

    crow::json::wvalue response;

    if (status)
    {
        response["success"] = true;
        response["message"] = "Notification created successfully";

        return crow::response(201, response);
    }

    response["success"] = false;
    response["message"] = "Failed to create notification";

    return crow::response(500, response);
}

crow::response NotificationController::getAllNotifications()
{
    auto notifications = m_service.getAllNotifications();

    crow::json::wvalue response;

    std::size_t index = 0;

    for (const auto& notification : notifications)
    {
        response[index]["id"] = notification.getId();
        response[index]["userId"] = notification.getUserId();
        response[index]["type"] = notification.getType();
        response[index]["message"] = notification.getMessage();
        response[index]["status"] = notification.getStatus();
        response[index]["createdAt"] = notification.getCreatedAt();

        ++index;
    }

    return crow::response(response);
}

crow::response NotificationController::getNotificationById(int id)
{
    auto notification = m_service.getNotificationById(id);

    if (!notification.has_value())
    {
        crow::json::wvalue response;

        response["success"] = false;
        response["message"] = "Notification not found";

        return crow::response(404, response);
    }

    crow::json::wvalue response;

    response["id"] = notification->getId();
    response["userId"] = notification->getUserId();
    response["type"] = notification->getType();
    response["message"] = notification->getMessage();
    response["status"] = notification->getStatus();
    response["createdAt"] = notification->getCreatedAt();

    return crow::response(response);
}

crow::response NotificationController::updateNotification(
    int id,
    const crow::request& req)
{
    auto json = crow::json::load(req.body);

    if (!json)
    {
        return crow::response(400, "Invalid JSON");
    }

    Notification notification(
        id,
        json["userId"].i(),
        json["type"].s(),
        json["message"].s(),
        json["status"].s(),
        json["createdAt"].s());

    bool status = m_service.updateNotification(notification);

    crow::json::wvalue response;

    if (status)
    {
        response["success"] = true;
        response["message"] = "Notification updated successfully";
    }
    else
    {
        response["success"] = false;
        response["message"] = "Notification not found";
    }

    return crow::response(response);
}

crow::response NotificationController::deleteNotification(int id)
{
    bool status = m_service.deleteNotification(id);

    crow::json::wvalue response;

    if (status)
    {
        response["success"] = true;
        response["message"] = "Notification deleted successfully";
    }
    else
    {
        response["success"] = false;
        response["message"] = "Notification not found";
    }

    return crow::response(response);
}
