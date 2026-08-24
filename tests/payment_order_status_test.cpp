#include "PaymentOrderStatus.h"

#include <cassert>
#include <iostream>

namespace
{
void expect(const std::string& current, const std::string& payment,
            bool accepted, bool updateRequired, const std::string& next)
{
    const auto result = paymentOrderTransition(current, payment);
    assert(result.accepted == accepted);
    assert(result.updateRequired == updateRequired);
    assert(result.nextStatus == next);
}
}

int main()
{
    expect("PENDING", "processing", true, true, "PAYMENT_PENDING");
    expect("PAYMENT_PENDING", "processing", true, false, "PAYMENT_PENDING");
    expect("PAYMENT_PENDING", "succeeded", true, true, "CONFIRMED");
    expect("CONFIRMED", "succeeded", true, false, "CONFIRMED");
    expect("DELIVERED", "succeeded", true, false, "DELIVERED");
    expect("PAYMENT_PENDING", "failed", true, true, "PAYMENT_FAILED");
    expect("PAYMENT_FAILED", "failed", true, false, "PAYMENT_FAILED");
    expect("PAYMENT_PENDING", "cancelled", true, true, "CANCELLED");
    expect("CANCELLED", "cancelled", true, false, "CANCELLED");
    expect("CONFIRMED", "failed", false, false, "");
    expect("PAYMENT_FAILED", "succeeded", false, false, "");
    expect("PAYMENT_PENDING", "unknown", false, false, "");

    std::cout << "payment/order transition tests passed\n";
    return 0;
}
