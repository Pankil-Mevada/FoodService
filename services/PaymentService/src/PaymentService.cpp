#include "PaymentService.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <chrono>

std::atomic<unsigned long long> PaymentService::m_transactionCounter{1};

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
        if (existing) {
            std::clog << "[payment-flow] create reused order=" << orderId
                      << " payment=" << existing->getId() << " idempotent=true" << std::endl;
            return existing;
        }
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

    if (!status) {
        std::clog << "[payment-flow] create failed order=" << orderId << " stage=database-save" << std::endl;
        return std::nullopt;
    }
    std::clog << "[payment-flow] create accepted order=" << orderId
              << " transaction=" << transactionId << " status=pending provider=test" << std::endl;

    bool notificationStatus =
       m_notificationClient.createNotification(
    userId,
    "Payment pending");

    if (!notificationStatus)
    {
        std::cout << "Notification failed" << std::endl;
    }

    // Indexed point lookup avoids copying/scanning the complete payment table
    // for every order (the former implementation became O(n^2) under load).
    auto persisted = m_repository.getPaymentByTransactionId(transactionId);
    return persisted ? persisted : std::optional<Payment>(payment);
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
    if (!allowed) {
        std::clog << "[payment-flow] transition rejected transaction=" << transactionId
                  << " from=" << old << " to=" << status << std::endl;
        return std::nullopt;
    }
    if (old != status && !m_repository.updateStatus(transactionId, status, providerPaymentId)) {
        std::clog << "[payment-flow] transition failed transaction=" << transactionId
                  << " stage=database-update" << std::endl;
        return std::nullopt;
    }
    std::clog << "[payment-flow] transition accepted transaction=" << transactionId
              << " from=" << old << " to=" << status << std::endl;
    return m_repository.getPaymentByTransactionId(transactionId);
}

std::optional<Payment> PaymentService::getPaymentByTransactionId(const std::string& transactionId)
{
    return m_repository.getPaymentByTransactionId(transactionId);
}

bool PaymentService::synchronizeOrder(const Payment& payment) const
{
    return m_orderClient.synchronizePaymentStatus(
        payment.getOrderId(), payment.getStatus());
}

std::optional<Payment> PaymentService::getPaymentByIdempotencyKey(const std::string& idempotencyKey)
{
    return m_repository.getPaymentByIdempotencyKey(idempotencyKey);
}

bool PaymentService::attachProviderOrder(const std::string& transactionId, const std::string& provider,
                                         const std::string& providerOrderId)
{
    return m_repository.updateProviderOrder(transactionId, provider, providerOrderId);
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
    const auto epochMicros = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::ostringstream oss;
    oss << "TXN-" << epochMicros << "-" << m_transactionCounter.fetch_add(1);
    return oss.str();
}
