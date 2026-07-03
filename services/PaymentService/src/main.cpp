#include <crow.h>

#include "Database.h"
#include "PaymentController.h"
#include "PaymentRepository.h"
#include "PaymentService.h"

int main()
{
    crow::SimpleApp app;

    Database database("payment.db");

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

    // Update Payment
    CROW_ROUTE(app, "/payments/<int>")
    .methods(crow::HTTPMethod::PUT)
    ([&controller](const crow::request& req, int id)
    {
        return controller.updatePayment(id, req);
    });

    // Delete Payment
    CROW_ROUTE(app, "/payments/<int>")
    .methods(crow::HTTPMethod::DELETE)
    ([&controller](int id)
    {
        return controller.deletePayment(id);
    });

    app.port(8083)
       .multithreaded()
       .run();

    return 0;
}
