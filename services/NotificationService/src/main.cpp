#include <crow.h>

#include "Database.h"
#include "NotificationController.h"
#include "NotificationRepository.h"
#include "NotificationService.h"

int main()
{
    crow::SimpleApp app;

    Database database("notification.db");

    database.createNotificationTable();

    NotificationRepository repository(database);
    NotificationService service(repository);
    NotificationController controller(service);

    // Health
    CROW_ROUTE(app, "/health")
    ([&controller]()
    {
        return controller.health();
    });

    // Create Notification
    CROW_ROUTE(app, "/notifications")
    .methods(crow::HTTPMethod::POST)
    ([&controller](const crow::request& req)
    {
        return controller.createNotification(req);
    });

    // Get All Notifications
    CROW_ROUTE(app, "/notifications")
    .methods(crow::HTTPMethod::GET)
    ([&controller]()
    {
        return controller.getAllNotifications();
    });

    // Get Notification By Id
    CROW_ROUTE(app, "/notifications/<int>")
    .methods(crow::HTTPMethod::GET)
    ([&controller](int id)
    {
        return controller.getNotificationById(id);
    });

    // Update Notification
    CROW_ROUTE(app, "/notifications/<int>")
    .methods(crow::HTTPMethod::PUT)
    ([&controller](const crow::request& req, int id)
    {
        return controller.updateNotification(id, req);
    });

    // Delete Notification
    CROW_ROUTE(app, "/notifications/<int>")
    .methods(crow::HTTPMethod::DELETE)
    ([&controller](int id)
    {
        return controller.deleteNotification(id);
    });

    app.port(8084)
       .multithreaded()
       .run();

    return 0;
}
