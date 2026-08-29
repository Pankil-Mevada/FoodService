#pragma once
#include "client/HttpClient.h"

class PaymentClient
{
public:
    PaymentClient();
    HttpResult createPayment(const std::string& body, const std::string& idempotencyKey);
    HttpResult getPayment(int id);
    HttpResult getPaymentForOrder(int orderId);
    HttpResult getPaymentStream(const std::string& orderId);
    HttpResult providerWebhook(const std::string& body, const std::string& secret);
    HttpResult createRazorpayOrder(const std::string& body);
    HttpResult verifyRazorpayPayment(const std::string& body);
private:
    std::string m_baseUrl;
    HttpClient m_http;
};
