#pragma once

#include <optional>
#include <vector>

#include "Database.h"
#include "Payment.h"

class PaymentRepository
{
public:

    explicit PaymentRepository(Database& database);

    bool savePayment(const Payment& payment);

    std::vector<Payment> getAllPayments();

    std::optional<Payment> getPaymentById(int id);
    std::optional<Payment> getPaymentByTransactionId(const std::string& transactionId);
    std::optional<Payment> getPaymentByIdempotencyKey(const std::string& key);
    bool updateStatus(const std::string& transactionId, const std::string& status,
                      const std::string& providerPaymentId);

    bool updatePayment(const Payment& payment);

    bool deletePayment(int id);

private:

    Database& m_database;
};
