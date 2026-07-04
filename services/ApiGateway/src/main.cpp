#include <crow.h>
#include "client/OrderClient.h"
int main()
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/health")
    ([]()
    {
        return "API Gateway is Healthy!";
    });

    OrderClient client; 

    CROW_ROUTE(app, "/orders")
        .methods(crow::HTTPMethod::POST)
    ([&client](const crow::request& req)
    {
        return crow::response(
            client.createOrder(req.body));
    });


    app.port(8085)
       .multithreaded()
       .run();

    return 0;
}