#include <crow.h>

#include "UserController.h"

int main()
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/health")
    (&UserController::health);

    CROW_ROUTE(app, "/register")
    .methods(crow::HTTPMethod::POST)
    (&UserController::registerUser);

    app.port(8080).multithreaded().run();

    return 0;
}
