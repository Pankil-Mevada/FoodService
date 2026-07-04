#pragma once

class PaymentClient
{
public:

    PaymentClient() = default;

    bool createPayment(
        int orderId,
        double amount);
};
