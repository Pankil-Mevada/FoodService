#include "PaymentService.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>

int PaymentService::m_transactionCounter = 1;

PaymentService::PaymentService(PaymentRepository& repository)
    : m_repository(repository)
{
}

std::optional<Payment> PaymentService::createPayment(
    int orderId,
    int userId,
    double amount,
    const std::string& paymentMethod,
    const std::string& idempotencyKey)
{
    if (!idempotencyKey.empty())
    {
        auto existing = m_repository.getPaymentByIdempotencyKey(idempotencyKey);
        if (existing) return existing;
    }
    const std::string transactionId = generateTransactionId();
    Payment payment(
        0,
        orderId,
        amount,
        paymentMethod,
        transactionId,
        "pending", idempotencyKey, "test", "", "");

    bool status = m_repository.savePayment(payment);

    if (!status) return std::nullopt;

    bool notificationStatus =
       m_notificationClient.createNotification(
    userId,
    "Payment pending");

    if (!notificationStatus)
    {
        std::cout << "Notification failed" << std::endl;
    }

    auto payments = m_repository.getAllPayments();
    for (auto it = payments.rbegin(); it != payments.rend(); ++it)
        if (it->getTransactionId() == transactionId) return *it;
    return payment;
}
std::vector<Payment> PaymentService::getAllPayments()
{
    return m_repository.getAllPayments();
}

std::optional<Payment> PaymentService::getPaymentById(int id)
{
    return m_repository.getPaymentById(id);
}

std::optional<Payment> PaymentService::getPaymentForOrder(int orderId)
{
    auto payments = m_repository.getAllPayments();
    for (auto it = payments.rbegin(); it != payments.rend(); ++it)
        if (it->getOrderId() == orderId) return *it;
    return std::nullopt;
}

std::optional<Payment> PaymentService::applyProviderEvent(const std::string& transactionId,
    const std::string& status, const std::string& providerPaymentId)
{
    auto current = m_repository.getPaymentByTransactionId(transactionId);
    if (!current) return std::nullopt;
    const auto& old = current->getStatus();
    const bool allowed = old == status ||
        (old == "pending" && (status == "processing" || status == "succeeded" || status == "failed" || status == "cancelled")) ||
        (old == "processing" && (status == "succeeded" || status == "failed" || status == "cancelled"));
    if (!allowed) return std::nullopt;
    if (old != status && !m_repository.updateStatus(transactionId, status, providerPaymentId)) return std::nullopt;
    return m_repository.getPaymentByTransactionId(transactionId);
}

bool PaymentService::updatePayment(const Payment& payment)
{
    return m_repository.updatePayment(payment);
}

bool PaymentService::deletePayment(int id)
{
    return m_repository.deletePayment(id);
}

std::string PaymentService::generateTransactionId()
{
    std::time_t now = std::time(nullptr);

    std::tm* timeInfo = std::localtime(&now);

    std::ostringstream oss;

    oss << "TXN-";

    oss << (timeInfo->tm_year + 1900);

    oss << std::setw(2)
        << std::setfill('0')
        << (timeInfo->tm_mon + 1);

    oss << std::setw(2)
        << std::setfill('0')
        << timeInfo->tm_mday;

    oss << "-";

    oss << std::setw(6)
        << std::setfill('0')
        << m_transactionCounter++;

    return oss.str();
}
