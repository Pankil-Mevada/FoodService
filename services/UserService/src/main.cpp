#include <crow.h>

#include "Database.h"
#include "UserController.h"
#include "UserRepository.h"
#include "UserService.h"

int main()
{
    // Create Crow application
    crow::SimpleApp app;

    // Open database
    Database database("foodservice.db");

    // Create users table
    database.execute(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "email TEXT UNIQUE NOT NULL,"
        "password TEXT NOT NULL"
        ");");

    // Dependency Injection
    UserRepository repository(database);
    UserService service(repository);
    UserController controller(service);

    // Health endpoint
    CROW_ROUTE(app, "/health")
    ([&controller]()
    {
        return controller.health();
    });

    // Register endpoint
    CROW_ROUTE(app, "/register")
    .methods(crow::HTTPMethod::POST)
    ([&controller](const crow::request& req)
    {
        return controller.registerUser(req);
    });

    // Start server
    app.port(8080)
       .multithreaded()
       .run();

    return 0;
}
