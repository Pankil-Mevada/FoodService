#pragma once

#include <optional>
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
    std::optional<Payment> getPaymentForOrder(int orderId);
    std::optional<Payment> applyProviderEvent(const std::string& transactionId,
        const std::string& status, const std::string& providerPaymentId);

    bool updatePayment(const Payment& payment);

    bool deletePayment(int id);

private:

    std::string generateTransactionId();

    PaymentRepository& m_repository;

    static int m_transactionCounter;


    NotificationClient m_notificationClient;
};
