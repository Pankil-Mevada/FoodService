#include <iostream>
#include <crow.h>  
int main()
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/health")
    ([]() {
        return "User Service is Healthy!";
    });

    app.port(8080).multithreaded().run();
    return 0;
}
