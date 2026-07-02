#include <crow.h>

#include "Database.h"
#include "RestaurantController.h"
#include "RestaurantRepository.h"
#include "RestaurantService.h"

int main()
{
    crow::SimpleApp app;

    Database database("restaurant.db");

    database.createRestaurantTable();

    RestaurantRepository repository(database);
    RestaurantService service(repository);
    RestaurantController controller(service);

    // Health Check
    CROW_ROUTE(app, "/health")
    ([&controller]()
    {
        return controller.health();
    });

    // Create Restaurant
    CROW_ROUTE(app, "/restaurants")
    .methods(crow::HTTPMethod::POST)
    ([&controller](const crow::request& req)
    {
        return controller.registerRestaurant(req);
    });

    // Get All Restaurants
    CROW_ROUTE(app, "/restaurants")
    .methods(crow::HTTPMethod::GET)
    ([&controller]()
    {
        return controller.getAllRestaurants();
    });

    // Get Restaurant By Id
    CROW_ROUTE(app, "/restaurants/<int>")
    .methods(crow::HTTPMethod::GET)
    ([&controller](int id)
    {
        return controller.getRestaurantById(id);
    });

    // Update Restaurant
    CROW_ROUTE(app, "/restaurants/<int>")
    .methods(crow::HTTPMethod::PUT)
    ([&controller](const crow::request& req, int id)
    {
        return controller.updateRestaurant(id, req);
    });

    // Delete Restaurant
    CROW_ROUTE(app, "/restaurants/<int>")
    .methods(crow::HTTPMethod::DELETE)
    ([&controller](int id)
    {
        return controller.deleteRestaurant(id);
    });

    app.port(8081)
       .multithreaded()
       .run();

    return 0;
}
