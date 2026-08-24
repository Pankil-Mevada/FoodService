#pragma once

#include <string>

class OrderClient
{
public:
    bool synchronizePaymentStatus(int orderId, const std::string& paymentStatus) const;
};
