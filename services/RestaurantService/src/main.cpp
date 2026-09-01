#include <crow.h>

#include "Database.h"
#include "RestaurantController.h"
#include "RestaurantRepository.h"
#include "RestaurantService.h"
#include "PartnerController.h"
#include "PartnerRepository.h"
#include "RequestLoggingMiddleware.h"

int main()
{
    crow::App<RequestLoggingMiddleware> app;
    RequestLoggingMiddleware::setServiceName("restaurant");

    Database database("restaurant.db");

    database.createRestaurantTable();

    RestaurantRepository repository(database);
    RestaurantService service(repository);
    RestaurantController controller(service);
    PartnerRepository partnerRepository(database);
    if (!partnerRepository.ready()) return 1;
    PartnerController partnerController(partnerRepository);

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

    CROW_ROUTE(app, "/partner/restaurants")
    .methods(crow::HTTPMethod::GET, crow::HTTPMethod::POST)
    ([&partnerController](const crow::request& req) {
        return req.method == crow::HTTPMethod::GET
            ? partnerController.listRestaurants(req)
            : partnerController.createRestaurant(req);
    });

    CROW_ROUTE(app, "/partner/restaurants/<int>")
    .methods(crow::HTTPMethod::GET, crow::HTTPMethod::PUT)
    ([&partnerController](const crow::request& req, int restaurantId) {
        return req.method == crow::HTTPMethod::GET
            ? partnerController.getRestaurant(req, restaurantId)
            : partnerController.updateRestaurant(req, restaurantId);
    });

    CROW_ROUTE(app, "/partner/restaurants/<int>/submit")
    .methods(crow::HTTPMethod::POST)
    ([&partnerController](const crow::request& req, int restaurantId) {
        return partnerController.submitRestaurant(req, restaurantId);
    });

    CROW_ROUTE(app, "/partner/restaurants/<int>/menu-items")
    .methods(crow::HTTPMethod::GET, crow::HTTPMethod::POST)
    ([&partnerController](const crow::request& req, int restaurantId) {
        return req.method == crow::HTTPMethod::GET
            ? partnerController.listMenuItems(req, restaurantId)
            : partnerController.createMenuItem(req, restaurantId);
    });

    CROW_ROUTE(app, "/partner/restaurants/<int>/menu-items/<int>")
    .methods(crow::HTTPMethod::PUT, crow::HTTPMethod::DELETE)
    ([&partnerController](const crow::request& req, int restaurantId, int itemId) {
        return req.method == crow::HTTPMethod::PUT
            ? partnerController.updateMenuItem(req, restaurantId, itemId)
            : partnerController.deleteMenuItem(req, restaurantId, itemId);
    });

    CROW_ROUTE(app, "/partner/restaurants/<int>/audit")
    .methods(crow::HTTPMethod::GET)
    ([&partnerController](const crow::request& req, int restaurantId) {
        return partnerController.listAudit(req, restaurantId);
    });

    app.loglevel(configuredLogLevel()).port(8081)
       .concurrency(64)
       .run();

    return 0;
}
