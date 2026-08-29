#include "client/PaymentClient.h"
#include <cstdlib>

PaymentClient::PaymentClient()
{
    const char* value = std::getenv("PAYMENT_SERVICE_URL");
    m_baseUrl = value ? value : "http://localhost:8083";
}
HttpResult PaymentClient::createPayment(const std::string& body, const std::string& key)
{ return m_http.post(m_baseUrl + "/payments", body, "", key.empty() ? std::vector<std::string>{} : std::vector<std::string>{"Idempotency-Key: " + key}); }
HttpResult PaymentClient::getPayment(int id) { return m_http.get(m_baseUrl + "/payments/" + std::to_string(id)); }
HttpResult PaymentClient::getPaymentForOrder(int id) { return m_http.get(m_baseUrl + "/payments/order/" + std::to_string(id)); }
HttpResult PaymentClient::getPaymentStream(const std::string& id) { return m_http.get(m_baseUrl + "/payments/stream?orderId=" + id); }
HttpResult PaymentClient::providerWebhook(const std::string& body, const std::string& secret)
{ return m_http.post(m_baseUrl + "/payments/webhooks/provider", body, "", {"X-Webhook-Secret: " + secret}); }
HttpResult PaymentClient::createRazorpayOrder(const std::string& body)
{ return m_http.post(m_baseUrl + "/payments/razorpay/order", body); }
HttpResult PaymentClient::verifyRazorpayPayment(const std::string& body)
{ return m_http.post(m_baseUrl + "/payments/razorpay/verify", body); }
