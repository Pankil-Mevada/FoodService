#pragma once

#include <string>

struct PaymentOrderTransition
{
    bool accepted{false};
    bool updateRequired{false};
    std::string nextStatus;
};

inline PaymentOrderTransition paymentOrderTransition(
    const std::string& currentOrderStatus,
    const std::string& paymentStatus)
{
    const bool awaitingPayment = currentOrderStatus == "PENDING" ||
        currentOrderStatus == "PAYMENT_PENDING";
    const bool paymentComplete = currentOrderStatus == "CONFIRMED" ||
        currentOrderStatus == "ASSIGNED" || currentOrderStatus == "PICKED_UP" ||
        currentOrderStatus == "ON_THE_WAY" || currentOrderStatus == "ARRIVING" ||
        currentOrderStatus == "DELIVERED";

    if (paymentStatus == "processing")
    {
        if (paymentComplete || currentOrderStatus == "PAYMENT_PENDING")
            return {true, false, currentOrderStatus};
        return awaitingPayment
            ? PaymentOrderTransition{true, true, "PAYMENT_PENDING"}
            : PaymentOrderTransition{};
    }
    if (paymentStatus == "succeeded")
    {
        if (paymentComplete) return {true, false, currentOrderStatus};
        return awaitingPayment
            ? PaymentOrderTransition{true, true, "CONFIRMED"}
            : PaymentOrderTransition{};
    }
    if (paymentStatus == "failed")
    {
        if (currentOrderStatus == "PAYMENT_FAILED")
            return {true, false, currentOrderStatus};
        return awaitingPayment
            ? PaymentOrderTransition{true, true, "PAYMENT_FAILED"}
            : PaymentOrderTransition{};
    }
    if (paymentStatus == "cancelled")
    {
        if (currentOrderStatus == "CANCELLED")
            return {true, false, currentOrderStatus};
        return awaitingPayment
            ? PaymentOrderTransition{true, true, "CANCELLED"}
            : PaymentOrderTransition{};
    }
    return {};
}
