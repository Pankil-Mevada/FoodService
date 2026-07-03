#include "PaymentController.h"

PaymentController::PaymentController(PaymentService& service)
    : m_service(service)
{
}

crow::response PaymentController::health()
{
    return crow::response("Payment Service is Healthy!");
}

crow::response PaymentController::createPayment(
    const crow::request& req)
{
    auto json = crow::json::load(req.body);

    if (!json)
    {
        return crow::response(400, "Invalid JSON");
    }

    bool status =
        m_service.createPayment(
            json["orderId"].i(),
            json["amount"].d(),
            json["paymentMethod"].s());

    crow::json::wvalue response;

    if (status)
    {
        response["success"] = true;
        response["message"] = "Payment created successfully";

        return crow::response(201, response);
    }

    response["success"] = false;
    response["message"] = "Payment failed";

    return crow::response(500, response);
}

crow::response PaymentController::getAllPayments()
{
    auto payments = m_service.getAllPayments();

    crow::json::wvalue response;

    size_t index = 0;

    for (const auto& payment : payments)
    {
        response[index]["id"] = payment.getId();
        response[index]["orderId"] = payment.getOrderId();
        response[index]["amount"] = payment.getAmount();
        response[index]["paymentMethod"] = payment.getPaymentMethod();
        response[index]["transactionId"] = payment.getTransactionId();
        response[index]["status"] = payment.getStatus();

        ++index;
    }

    return crow::response(response);
}

crow::response PaymentController::getPaymentById(int id)
{
    auto payment = m_service.getPaymentById(id);

    if (!payment.has_value())
    {
        crow::json::wvalue response;

        response["success"] = false;
        response["message"] = "Payment not found";

        return crow::response(404, response);
    }

    crow::json::wvalue response;

    response["id"] = payment->getId();
    response["orderId"] = payment->getOrderId();
    response["amount"] = payment->getAmount();
    response["paymentMethod"] = payment->getPaymentMethod();
    response["transactionId"] = payment->getTransactionId();
    response["status"] = payment->getStatus();

    return crow::response(response);
}

crow::response PaymentController::updatePayment(
    int id,
    const crow::request& req)
{
    auto json = crow::json::load(req.body);

    if (!json)
    {
        return crow::response(400, "Invalid JSON");
    }

    Payment payment(
        id,
        json["orderId"].i(),
        json["amount"].d(),
        json["paymentMethod"].s(),
        json["transactionId"].s(),
        json["status"].s());

    bool status = m_service.updatePayment(payment);

    crow::json::wvalue response;

    if (status)
    {
        response["success"] = true;
        response["message"] = "Payment updated successfully";
    }
    else
    {
        response["success"] = false;
        response["message"] = "Payment not found";
    }

    return crow::response(response);
}

crow::response PaymentController::deletePayment(int id)
{
    bool status = m_service.deletePayment(id);

    crow::json::wvalue response;

    if (status)
    {
        response["success"] = true;
        response["message"] = "Payment deleted successfully";
    }
    else
    {
        response["success"] = false;
        response["message"] = "Payment not found";
    }

    return crow::response(response);
}
