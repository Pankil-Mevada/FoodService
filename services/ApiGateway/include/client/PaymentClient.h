#pragma once
#include "client/HttpClient.h"

class PaymentClient
{
public:
    PaymentClient();
    std::string createPayment(const std::string& body, const std::string& idempotencyKey);
    std::string getPayment(int id);
    std::string getPaymentForOrder(int orderId);
    std::string getPaymentStream(const std::string& orderId);
    std::string providerWebhook(const std::string& body, const std::string& secret);
    std::string createRazorpayOrder(const std::string& body);
    std::string verifyRazorpayPayment(const std::string& body);
private:
    std::string m_baseUrl;
    HttpClient m_http;
};
