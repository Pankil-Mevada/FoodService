#include <crow.h>
#include "JwtMiddleware.h"
#include "Database.h"
#include "UserController.h"
#include "UserRepository.h"
#include "UserService.h"

int main()
{
    // Create Crow application
//    crow::SimpleApp app;
	using App = crow::App<JwtMiddleware>;

	App app;
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


    CROW_ROUTE(app, "/users/<int>")
	    .methods(crow::HTTPMethod::DELETE)
	    ([&controller](int id)
	     {
	     return controller.deleteUser(id);
	     });
    CROW_ROUTE(app, "/users/<int>")
	    .methods(crow::HTTPMethod::PUT)
	    ([&controller](const crow::request& req, int id)
	     {
	     return controller.updateUser(id, req);
	     });
    CROW_ROUTE(app, "/login")
	    .methods(crow::HTTPMethod::POST)
	    ([&controller](const crow::request& req)
	     {
	     return controller.login(req);
	     });
    CROW_ROUTE(app,"/users/<int>")
	    .methods(crow::HTTPMethod::GET)
	    ([&controller](int id)
	     {
	     return controller.getUserById(id);
	     });
    // Health endpoint
    CROW_ROUTE(app, "/health")
	    ([&controller]()
	     {
	     return controller.health();
	     });

    CROW_ROUTE(app, "/users")
	    .methods(crow::HTTPMethod::GET)
	    ([&controller]()
	     {
	     return controller.getAllUsers();
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
