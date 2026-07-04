#pragma once

class PaymentClient
{
public:

    PaymentClient() = default;

    bool createPayment(
    int orderId,
    int userId,
    double amount);
};
