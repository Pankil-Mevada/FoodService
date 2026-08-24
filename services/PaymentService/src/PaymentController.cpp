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
crow::response orderSyncFailure(const Payment& payment)
{
    crow::json::wvalue response;
    response["success"] = false;
    response["message"] = "Payment was stored but Order Service synchronization failed; retry this provider event safely";
    response["payment"] = paymentJson(payment);
    return crow::response(502, response);
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
    if (!json || !json.has("transactionId") || !json.has("status")) {
        CROW_LOG_WARNING << "Provider webhook rejected: invalid payload";
        return crow::response(400, "Invalid provider event");
    }
    const std::string status = json["status"].s();
    if (status != "processing" && status != "succeeded" && status != "failed" && status != "cancelled")
        return crow::response(422, "Unsupported payment status");
    std::string providerId = json.has("providerPaymentId") ? std::string(json["providerPaymentId"].s()) : "";
    auto payment = m_service.applyProviderEvent(json["transactionId"].s(), status, providerId);
    if (!payment) { CROW_LOG_WARNING << "Provider webhook transition rejected"; return crow::response(409, "Payment not found or transition rejected"); }
    if (!m_service.synchronizeOrder(*payment)) {
        CROW_LOG_ERROR << "Provider webhook stored payment but order synchronization failed transaction "
                       << json["transactionId"].s();
        return orderSyncFailure(*payment);
    }
    CROW_LOG_INFO << "Provider webhook applied transaction " << json["transactionId"].s() << " status " << status;
    return crow::response(paymentJson(*payment));
}

crow::response PaymentController::createRazorpayOrder(const crow::request& req)
{
    const auto json = crow::json::load(req.body);
    if (!json || !json.has("transactionId")) return crow::response(422, "transactionId is required");
    const std::string transactionId = json["transactionId"].s();
    CROW_LOG_INFO << "Razorpay order requested for transaction " << transactionId;
    auto existing = m_service.getPaymentByTransactionId(transactionId);
    if (!existing) { CROW_LOG_WARNING << "Razorpay order rejected: payment not found"; return crow::response(404, "Payment not found"); }
    if (existing->getStatus() != "pending") { CROW_LOG_WARNING << "Razorpay order rejected: payment not pending"; return crow::response(409, "Payment is not pending"); }
    if (existing->getProvider() == "razorpay" && existing->getProviderPaymentId().rfind("order_", 0) == 0)
    {
        crow::json::wvalue out; out["keyId"] = m_razorpay.keyId(); out["providerOrderId"] = existing->getProviderPaymentId();
        out["amount"] = static_cast<long long>(std::llround(existing->getAmount() * 100.0)); out["currency"] = "INR";
        return crow::response(out);
    }
    std::string error;
    const long long amountPaise = static_cast<long long>(std::llround(existing->getAmount() * 100.0));
    auto order = m_razorpay.createOrder(amountPaise, transactionId, error);
    if (!order) { CROW_LOG_ERROR << "Razorpay order creation failed for transaction " << transactionId; return crow::response(502, error); }
    if (!m_service.attachProviderOrder(transactionId, "razorpay", order->id)) return crow::response(500, "Could not save Razorpay order");
    crow::json::wvalue out; out["keyId"] = m_razorpay.keyId(); out["providerOrderId"] = order->id;
    out["amount"] = order->amount; out["currency"] = order->currency;
    return crow::response(201, out);
}

crow::response PaymentController::verifyRazorpayPayment(const crow::request& req)
{
    const auto json = crow::json::load(req.body);
    if (!json || !json.has("transactionId") || !json.has("razorpay_order_id") ||
        !json.has("razorpay_payment_id") || !json.has("razorpay_signature"))
        return crow::response(422, "Incomplete Razorpay verification payload");
    const std::string transactionId = json["transactionId"].s();
    CROW_LOG_INFO << "Razorpay verification requested for transaction " << transactionId;
    const std::string orderId = json["razorpay_order_id"].s();
    const std::string paymentId = json["razorpay_payment_id"].s();
    auto payment = m_service.getPaymentByTransactionId(transactionId);
    if (!payment || payment->getProvider() != "razorpay" || payment->getProviderPaymentId() != orderId) {
        CROW_LOG_WARNING << "Razorpay verification rejected: provider order mismatch transaction " << transactionId;
        return crow::response(409, "Razorpay order does not match this payment");
    }
    if (!m_razorpay.verifyPaymentSignature(orderId, paymentId, json["razorpay_signature"].s())) {
        CROW_LOG_WARNING << "Razorpay verification rejected: invalid signature transaction " << transactionId;
        return crow::response(401, "Invalid Razorpay payment signature");
    }
    auto updated = m_service.applyProviderEvent(transactionId, "succeeded", paymentId);
    if (!updated) return crow::response(409, "Payment transition rejected");
    if (!m_service.synchronizeOrder(*updated)) {
        CROW_LOG_ERROR << "Razorpay payment stored but order synchronization failed transaction " << transactionId;
        return orderSyncFailure(*updated);
    }
    CROW_LOG_INFO << "Razorpay payment verified transaction " << transactionId << " status succeeded";
    return crow::response(paymentJson(*updated));
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
