#include <crow.h>

#include "Database.h"
#include "RestaurantController.h"
#include "RestaurantRepository.h"
#include "RestaurantService.h"

int main()
{
    // Create Crow application
    crow::SimpleApp app;

    // Open database
    Database database("restaurant.db");

    // Create users table
    database.createRestaurantTable();
    // Dependency Injection
    RestaurantRepository repository(database);
    RestaurantService service(repository);
    RestaurantController controller(service);


    CROW_ROUTE(app, "/users/<int>")
	    .methods(crow::HTTPMethod::DELETE)
	    ([&controller](int id)
	     {
	     return controller.deleteRestaurant(id);
	     });
    CROW_ROUTE(app, "/users/<int>")
	    .methods(crow::HTTPMethod::PUT)
	    ([&controller](const crow::request& req, int id)
	     {
	     return controller.updateRestaurant(id, req);
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
	     return controller.getRestaurantById(id);
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
	     return controller.getAllRestaurants();
	     });

    // Register endpoint
    CROW_ROUTE(app, "/register")
    .methods(crow::HTTPMethod::POST)
    ([&controller](const crow::request& req)
    {
        return controller.registerRestaurant(req);
    });

    // Start server
    app.port(8080)
       .multithreaded()
       .run();

    return 0;
}
