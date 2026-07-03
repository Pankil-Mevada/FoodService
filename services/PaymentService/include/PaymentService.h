#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Payment.h"
#include "PaymentRepository.h"

class PaymentService
{
public:

    explicit PaymentService(PaymentRepository& repository);

    bool createPayment(
        int orderId,
        double amount,
        const std::string& paymentMethod);

    std::vector<Payment> getAllPayments();

    std::optional<Payment> getPaymentById(int id);

    bool updatePayment(const Payment& payment);

    bool deletePayment(int id);

private:

    std::string generateTransactionId();

    PaymentRepository& m_repository;

    static int m_transactionCounter;
};
