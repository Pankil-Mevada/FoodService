#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace restaurant_order
{
enum class Status
{
    New,
    Accepted,
    Preparing,
    ReadyForPickup,
    HandedOff,
    Unknown
};

struct TransitionDecision
{
    bool allowed{false};
    std::string message;
};

inline std::optional<Status> parseStatus(std::string_view value)
{
    if (value == "NEW") return Status::New;
    if (value == "ACCEPTED") return Status::Accepted;
    if (value == "PREPARING") return Status::Preparing;
    if (value == "READY_FOR_PICKUP") return Status::ReadyForPickup;
    if (value == "HANDED_OFF") return Status::HandedOff;
    return std::nullopt;
}

inline std::string_view statusName(Status value)
{
    switch (value)
    {
        case Status::New: return "NEW";
        case Status::Accepted: return "ACCEPTED";
        case Status::Preparing: return "PREPARING";
        case Status::ReadyForPickup: return "READY_FOR_PICKUP";
        case Status::HandedOff: return "HANDED_OFF";
        default: return "UNKNOWN";
    }
}

inline bool isPaidOrderStatus(std::string_view value)
{
    return value == "CONFIRMED" || value == "ASSIGNED" ||
        value == "PICKED_UP" || value == "ON_THE_WAY" ||
        value == "ARRIVING" || value == "DELIVERED";
}

inline bool driverAssigned(std::string_view value)
{
    return value == "ASSIGNED" || value == "PICKED_UP" ||
        value == "ON_THE_WAY" || value == "ARRIVING" ||
        value == "DELIVERED";
}

inline Status initialStatus(std::string_view orderStatus)
{
    if (orderStatus == "ASSIGNED") return Status::ReadyForPickup;
    if (orderStatus == "PICKED_UP" || orderStatus == "ON_THE_WAY" ||
        orderStatus == "ARRIVING" || orderStatus == "DELIVERED")
        return Status::HandedOff;
    return Status::New;
}

inline std::optional<Status> nextStatus(Status current)
{
    switch (current)
    {
        case Status::New: return Status::Accepted;
        case Status::Accepted: return Status::Preparing;
        case Status::Preparing: return Status::ReadyForPickup;
        case Status::ReadyForPickup: return Status::HandedOff;
        default: return std::nullopt;
    }
}

inline TransitionDecision canTransition(
    Status current,
    Status target,
    bool hasAssignedDriver)
{
    const auto expected = nextStatus(current);
    if (!expected || *expected != target)
        return {false, "Kitchen status must advance one step at a time"};
    if (target == Status::HandedOff && !hasAssignedDriver)
        return {false, "A driver must be assigned before handoff"};
    return {true, {}};
}
}
