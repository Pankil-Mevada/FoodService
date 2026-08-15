#pragma once

#include <string>

class Payment
{
public:

    Payment() = default;

    Payment(
        int id,
        int orderId,
        double amount,
        const std::string& paymentMethod,
        const std::string& transactionId,
        const std::string& status,
        const std::string& idempotencyKey = "",
        const std::string& provider = "test",
        const std::string& providerPaymentId = "",
        const std::string& updatedAt = "");

    int getId() const;

    int getOrderId() const;

    double getAmount() const;

    const std::string& getPaymentMethod() const;

    const std::string& getTransactionId() const;

    const std::string& getStatus() const;
    const std::string& getIdempotencyKey() const;
    const std::string& getProvider() const;
    const std::string& getProviderPaymentId() const;
    const std::string& getUpdatedAt() const;

private:

    int m_id{0};

    int m_orderId{0};

    double m_amount{0.0};

    std::string m_paymentMethod;

    std::string m_transactionId;

    std::string m_status;
    std::string m_idempotencyKey;
    std::string m_provider;
    std::string m_providerPaymentId;
    std::string m_updatedAt;
};
