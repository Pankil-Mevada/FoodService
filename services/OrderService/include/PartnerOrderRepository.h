#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Database.h"

enum class PartnerOrderWriteResult
{
    Updated,
    AlreadyApplied,
    NotFound,
    VersionConflict,
    InvalidTransition,
    IdempotencyConflict,
    StorageError
};

struct PartnerOrderView
{
    int id{0};
    int restaurantId{0};
    double totalAmount{0.0};
    std::string orderStatus;
    std::string restaurantStatus;
    std::string deliveryAddress;
    std::string itemSummary;
    int preparationMinutes{20};
    int version{0};
    long long updatedEpoch{0};
};

struct PartnerOrderWriteOutcome
{
    PartnerOrderWriteResult result{PartnerOrderWriteResult::StorageError};
    std::optional<PartnerOrderView> order;
    std::string message;
};

class PartnerOrderRepository
{
public:
    explicit PartnerOrderRepository(Database& database);

    [[nodiscard]] bool ready() const;
    std::vector<PartnerOrderView> listPaidOrders(int restaurantId, int limit = 100);
    PartnerOrderWriteOutcome transition(
        int restaurantId,
        int orderId,
        int actorUserId,
        const std::string& targetStatus,
        int expectedVersion,
        int preparationMinutes,
        const std::string& idempotencyKey,
        const std::string& correlationId);

private:
    Database& m_database;
    bool m_ready{false};
};
