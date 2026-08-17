#pragma once
#include <optional>
#include <string>

struct RazorpayOrder { std::string id; long long amount; std::string currency; };

class RazorpayClient
{
public:
    bool configured() const;
    std::string keyId() const;
    std::optional<RazorpayOrder> createOrder(long long amountPaise, const std::string& receipt,
                                             std::string& error) const;
    bool verifyPaymentSignature(const std::string& orderId, const std::string& paymentId,
                                const std::string& signature) const;
};
