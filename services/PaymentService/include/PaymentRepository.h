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

    bool updatePayment(const Payment& payment);

    bool deletePayment(int id);

private:

    Database& m_database;
};
