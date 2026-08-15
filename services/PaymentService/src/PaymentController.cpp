#include "PaymentController.h"
#include <algorithm>
#include <cstdlib>
#include <cmath>

namespace
{
crow::json::wvalue paymentJson(const Payment& payment)
{
    crow::json::wvalue out;
    out["id"] = payment.getId(); out["orderId"] = payment.getOrderId();
    out["amount"] = payment.getAmount(); out["paymentMethod"] = payment.getPaymentMethod();
    out["transactionId"] = payment.getTransactionId(); out["status"] = payment.getStatus();
    out["provider"] = payment.getProvider(); out["providerPaymentId"] = payment.getProviderPaymentId();
    out["updatedAt"] = payment.getUpdatedAt();
    return out;
}
bool safeEqual(const std::string& a, const std::string& b)
{
    size_t difference = a.size() ^ b.size();
    for (size_t i = 0; i < std::max(a.size(), b.size()); ++i)
        difference |= static_cast<unsigned char>(i < a.size() ? a[i] : 0) ^ static_cast<unsigned char>(i < b.size() ? b[i] : 0);
    return difference == 0;
}
}

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

    if (!json.has("orderId") || !json.has("userId") || !json.has("amount") || !json.has("paymentMethod"))
        return crow::response(422, "Missing orderId, userId, amount or paymentMethod");
    const int orderId = json["orderId"].i(); const int userId = json["userId"].i();
    const double amount = json["amount"].d(); const std::string method = json["paymentMethod"].s();
    if (orderId <= 0 || userId <= 0 || !std::isfinite(amount) || amount <= 0.0 || amount > 1000000.0 || method.empty())
        return crow::response(422, "Invalid payment values");
    std::string key = req.get_header_value("Idempotency-Key");
    if (key.empty() && json.has("idempotencyKey")) key = json["idempotencyKey"].s();
    if (key.size() > 128) return crow::response(422, "Idempotency key is too long");
    auto payment = m_service.createPayment(orderId, userId, amount, method, key);

    crow::json::wvalue response;

    if (payment)
    {
        response["success"] = true;
        response["message"] = "Payment created successfully";
        response["payment"] = paymentJson(*payment);

        return crow::response(201, response);
    }

    response["success"] = false;
    response["message"] = "Payment failed";

    return crow::response(500, response);
}

crow::response PaymentController::getPaymentForOrder(int orderId)
{
    auto payment = m_service.getPaymentForOrder(orderId);
    if (!payment) return crow::response(404, "Payment not found");
    return crow::response(paymentJson(*payment));
}

crow::response PaymentController::paymentStream(const crow::request& req)
{
    const char* raw = req.url_params.get("orderId");
    if (!raw) return crow::response(422, "orderId query parameter is required");
    int orderId = 0; try { orderId = std::stoi(raw); } catch (...) { return crow::response(422, "Invalid orderId"); }
    auto payment = m_service.getPaymentForOrder(orderId);
    if (!payment) return crow::response(404, "Payment not found");
    crow::response response;
    response.code = 200;
    response.set_header("Content-Type", "text/event-stream");
    response.set_header("Cache-Control", "no-cache");
    response.set_header("Connection", "keep-alive");
    response.body = "retry: 2000\nevent: payment-status\ndata: " + paymentJson(*payment).dump() + "\n\n";
    return response;
}

crow::response PaymentController::providerWebhook(const crow::request& req)
{
    const char* configured = std::getenv("PAYMENT_WEBHOOK_SECRET");
    const std::string expected = configured ? configured : "test-webhook-secret";
    if (!safeEqual(req.get_header_value("X-Webhook-Secret"), expected)) return crow::response(401, "Invalid webhook secret");
    auto json = crow::json::load(req.body);
    if (!json || !json.has("transactionId") || !json.has("status")) return crow::response(400, "Invalid provider event");
    const std::string status = json["status"].s();
    if (status != "processing" && status != "succeeded" && status != "failed" && status != "cancelled")
        return crow::response(422, "Unsupported payment status");
    std::string providerId = json.has("providerPaymentId") ? std::string(json["providerPaymentId"].s()) : "";
    auto payment = m_service.applyProviderEvent(json["transactionId"].s(), status, providerId);
    if (!payment) return crow::response(409, "Payment not found or transition rejected");
    return crow::response(paymentJson(*payment));
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
