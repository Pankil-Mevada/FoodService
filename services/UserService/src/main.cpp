#include <crow.h>
#include "Database.h"
#include "UserController.h"

int main()
{
    crow::SimpleApp app;

    Database database("users.db");

database.execute(
    "CREATE TABLE IF NOT EXISTS users("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT NOT NULL,"
    "email TEXT UNIQUE NOT NULL,"
    "password TEXT NOT NULL"
    ");");

    CROW_ROUTE(app, "/health")
    (&UserController::health);

    CROW_ROUTE(app, "/register")
    .methods(crow::HTTPMethod::POST)
    (&UserController::registerUser);

    app.port(8080).multithreaded().run();

    return 0;
}
