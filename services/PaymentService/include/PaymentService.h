#pragma once

#include <optional>
#include <atomic>
#include <string>
#include <vector>

#include "Payment.h"
#include "PaymentRepository.h"
#include "client/NotificationClient.h"

class PaymentService
{
public:

    explicit PaymentService(PaymentRepository& repository);

    std::optional<Payment> createPayment(
    int orderId,
    int userId,
    double amount,
    const std::string& paymentMethod,
    const std::string& idempotencyKey);

    std::vector<Payment> getAllPayments();

    std::optional<Payment> getPaymentById(int id);
    std::optional<Payment> getPaymentByTransactionId(const std::string& transactionId);
    std::optional<Payment> getPaymentByIdempotencyKey(const std::string& idempotencyKey);
    std::optional<Payment> getPaymentForOrder(int orderId);
    std::optional<Payment> applyProviderEvent(const std::string& transactionId,
        const std::string& status, const std::string& providerPaymentId);
    bool attachProviderOrder(const std::string& transactionId, const std::string& provider,
                             const std::string& providerOrderId);

    bool updatePayment(const Payment& payment);

    bool deletePayment(int id);

private:

    std::string generateTransactionId();

    PaymentRepository& m_repository;

    static std::atomic<unsigned long long> m_transactionCounter;


    NotificationClient m_notificationClient;
};
