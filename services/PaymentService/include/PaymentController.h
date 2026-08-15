#pragma once

#include <crow.h>

#include "PaymentService.h"

class PaymentController
{
public:

    explicit PaymentController(PaymentService& service);

    crow::response health();

    crow::response createPayment(const crow::request& req);

    crow::response getAllPayments();

    crow::response getPaymentById(int id);
    crow::response getPaymentForOrder(int orderId);
    crow::response paymentStream(const crow::request& req);
    crow::response providerWebhook(const crow::request& req);

    crow::response updatePayment(
        int id,
        const crow::request& req);

    crow::response deletePayment(int id);

private:

    PaymentService& m_service;
};
