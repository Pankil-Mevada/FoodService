#include "PartnerOrderController.h"

#include "RestaurantOrderWorkflow.h"

#include <algorithm>
#include <charconv>

namespace
{
crow::response errorResponse(int status, const std::string& message)
{
    crow::json::wvalue body;
    body["success"] = false;
    body["message"] = message;
    return crow::response(status, body);
}

std::optional<int> partnerUserId(const crow::request& req)
{
    const std::string value = req.get_header_value("X-Partner-User-ID");
    int result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || result <= 0)
        return std::nullopt;
    return result;
}

crow::json::wvalue orderJson(const PartnerOrderView& order)
{
    crow::json::wvalue body;
    body["id"] = order.id;
    body["restaurantId"] = order.restaurantId;
    body["totalAmount"] = order.totalAmount;
    body["orderStatus"] = order.orderStatus;
    body["restaurantStatus"] = order.restaurantStatus;
    body["deliveryAddress"] = order.deliveryAddress;
    body["itemSummary"] = order.itemSummary;
    body["preparationMinutes"] = order.preparationMinutes;
    body["version"] = order.version;
    body["updatedEpoch"] = order.updatedEpoch;
    const auto status = restaurant_order::parseStatus(order.restaurantStatus);
    const auto next = status ? restaurant_order::nextStatus(*status) : std::nullopt;
    body["nextStatus"] = next ? std::string(restaurant_order::statusName(*next)) : "";
    body["driverAssigned"] = restaurant_order::driverAssigned(order.orderStatus);
    return body;
}
}

PartnerOrderController::PartnerOrderController(PartnerOrderRepository& repository)
    : m_repository(repository)
{
}

crow::response PartnerOrderController::listOrders(
    const crow::request& req,
    int restaurantId)
{
    if (!partnerUserId(req)) return errorResponse(401, "Trusted partner identity is required");
    crow::json::wvalue response = crow::json::wvalue::list();
    std::size_t index = 0;
    for (const auto& order : m_repository.listPaidOrders(restaurantId))
        response[index++] = orderJson(order);
    return crow::response(response);
}

crow::response PartnerOrderController::transitionOrder(
    const crow::request& req,
    int restaurantId,
    int orderId)
{
    const auto actor = partnerUserId(req);
    if (!actor) return errorResponse(401, "Trusted partner identity is required");
    const std::string idempotencyKey = req.get_header_value("Idempotency-Key");
    if (idempotencyKey.size() < 8 || idempotencyKey.size() > 128)
        return errorResponse(400, "A valid Idempotency-Key is required");
    const auto input = crow::json::load(req.body);
    if (!input || input.t() != crow::json::type::Object ||
        !input.has("status") || !input.has("expectedVersion"))
        return errorResponse(400, "status and expectedVersion are required");
    if (input["status"].t() != crow::json::type::String ||
        input["expectedVersion"].t() != crow::json::type::Number ||
        (input.has("preparationMinutes") &&
            input["preparationMinutes"].t() != crow::json::type::Number))
        return errorResponse(400, "Restaurant order fields have invalid types");
    const std::string target(input["status"].s());
    const int expectedVersion = input["expectedVersion"].i();
    if (expectedVersion < 0)
        return errorResponse(422, "expectedVersion must not be negative");
    int preparationMinutes = input.has("preparationMinutes")
        ? input["preparationMinutes"].i() : 20;
    if (target == "ACCEPTED" && (preparationMinutes < 1 || preparationMinutes > 240))
        return errorResponse(422, "Preparation time must be between 1 and 240 minutes");

    auto outcome = m_repository.transition(
        restaurantId, orderId, *actor, target, expectedVersion,
        preparationMinutes, idempotencyKey, req.get_header_value("X-Correlation-ID"));
    switch (outcome.result)
    {
        case PartnerOrderWriteResult::Updated:
        case PartnerOrderWriteResult::AlreadyApplied:
        {
            crow::json::wvalue body = orderJson(*outcome.order);
            body["success"] = true;
            body["idempotentReplay"] = outcome.result == PartnerOrderWriteResult::AlreadyApplied;
            return crow::response(200, body);
        }
        case PartnerOrderWriteResult::NotFound:
            return errorResponse(404, outcome.message);
        case PartnerOrderWriteResult::VersionConflict:
        case PartnerOrderWriteResult::InvalidTransition:
        case PartnerOrderWriteResult::IdempotencyConflict:
            return errorResponse(409, outcome.message);
        default:
            return errorResponse(500, outcome.message.empty()
                ? "Restaurant order update failed" : outcome.message);
    }
}
