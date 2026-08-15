#include <crow.h>

#include "Database.h"
#include "PaymentController.h"
#include "PaymentRepository.h"
#include "PaymentService.h"
#include <cstdlib>

int main()
{
    crow::SimpleApp app;

    const char* databasePath = std::getenv("PAYMENT_DATABASE_PATH");
    Database database(databasePath ? databasePath : "payment.db");

    database.createPaymentTable();

    PaymentRepository repository(database);
    PaymentService service(repository);
    PaymentController controller(service);

    // Health
    CROW_ROUTE(app, "/health")
    ([&controller]()
    {
        return controller.health();
    });

    // Create Payment
    CROW_ROUTE(app, "/payments")
    .methods(crow::HTTPMethod::POST)
    ([&controller](const crow::request& req)
    {
        return controller.createPayment(req);
    });

    // Get All Payments
    CROW_ROUTE(app, "/payments")
    .methods(crow::HTTPMethod::GET)
    ([&controller]()
    {
        return controller.getAllPayments();
    });

    // Get Payment By Id
    CROW_ROUTE(app, "/payments/<int>")
    .methods(crow::HTTPMethod::GET)
    ([&controller](int id)
    {
        return controller.getPaymentById(id);
    });

    CROW_ROUTE(app, "/payments/order/<int>")
    .methods(crow::HTTPMethod::GET)
    ([&controller](int orderId) { return controller.getPaymentForOrder(orderId); });

    CROW_ROUTE(app, "/payments/stream")
    .methods(crow::HTTPMethod::GET)
    ([&controller](const crow::request& req) { return controller.paymentStream(req); });

    CROW_ROUTE(app, "/payments/webhooks/provider")
    .methods(crow::HTTPMethod::POST)
    ([&controller](const crow::request& req) { return controller.providerWebhook(req); });

    const char* portValue = std::getenv("PAYMENT_SERVICE_PORT");
    const unsigned short port = portValue ? static_cast<unsigned short>(std::stoi(portValue)) : 8083;
    app.port(port)
       .multithreaded()
       .run();

    return 0;
}
