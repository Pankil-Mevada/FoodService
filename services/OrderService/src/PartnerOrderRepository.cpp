#include "PartnerOrderRepository.h"

#include "RestaurantOrderWorkflow.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <sqlite3.h>

namespace
{
long long nowEpoch()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string textColumn(sqlite3_stmt* statement, int column)
{
    const auto* value = sqlite3_column_text(statement, column);
    return value ? reinterpret_cast<const char*>(value) : std::string{};
}

PartnerOrderView readOrder(sqlite3_stmt* statement)
{
    PartnerOrderView value;
    value.id = sqlite3_column_int(statement, 0);
    value.restaurantId = sqlite3_column_int(statement, 1);
    value.totalAmount = sqlite3_column_double(statement, 2);
    value.orderStatus = textColumn(statement, 3);
    value.deliveryAddress = textColumn(statement, 4);
    value.itemSummary = textColumn(statement, 5);
    const std::string storedStatus = textColumn(statement, 6);
    value.restaurantStatus = storedStatus.empty()
        ? std::string(restaurant_order::statusName(
            restaurant_order::initialStatus(value.orderStatus)))
        : storedStatus;
    value.preparationMinutes = sqlite3_column_type(statement, 7) == SQLITE_NULL
        ? 20 : sqlite3_column_int(statement, 7);
    value.version = sqlite3_column_type(statement, 8) == SQLITE_NULL
        ? 0 : sqlite3_column_int(statement, 8);
    value.updatedEpoch = sqlite3_column_type(statement, 9) == SQLITE_NULL
        ? 0 : sqlite3_column_int64(statement, 9);
    return value;
}

std::optional<PartnerOrderView> findPaidOrder(
    sqlite3* connection,
    int restaurantId,
    int orderId)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT o.id,o.restaurant_id,o.total_amount,o.status,"
        "o.delivery_address,o.item_summary,w.status,w.preparation_minutes,"
        "w.version,w.updated_epoch FROM orders o LEFT JOIN "
        "restaurant_order_workflows w ON w.order_id=o.id "
        "WHERE o.restaurant_id=? AND o.id=? AND o.status IN "
        "('CONFIRMED','ASSIGNED','PICKED_UP','ON_THE_WAY','ARRIVING','DELIVERED');";
    if (sqlite3_prepare_v2(connection, sql, -1, &statement, nullptr) != SQLITE_OK)
        return std::nullopt;
    sqlite3_bind_int(statement, 1, restaurantId);
    sqlite3_bind_int(statement, 2, orderId);
    std::optional<PartnerOrderView> result;
    if (sqlite3_step(statement) == SQLITE_ROW) result = readOrder(statement);
    sqlite3_finalize(statement);
    return result;
}

void rollback(sqlite3* connection)
{
    sqlite3_exec(connection, "ROLLBACK;", nullptr, nullptr, nullptr);
}
}

PartnerOrderRepository::PartnerOrderRepository(Database& database)
    : m_database(database)
{
    std::lock_guard<std::recursive_mutex> lock(m_database.mutex());
    const char* schema = R"sql(
        CREATE TABLE IF NOT EXISTS restaurant_order_workflows (
            order_id INTEGER PRIMARY KEY,
            restaurant_id INTEGER NOT NULL,
            status TEXT NOT NULL,
            preparation_minutes INTEGER NOT NULL DEFAULT 20,
            version INTEGER NOT NULL DEFAULT 0,
            updated_epoch INTEGER NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_restaurant_order_workflows_restaurant
            ON restaurant_order_workflows(restaurant_id,order_id DESC);
        CREATE TABLE IF NOT EXISTS restaurant_order_commands (
            restaurant_id INTEGER NOT NULL,
            idempotency_key TEXT NOT NULL,
            order_id INTEGER NOT NULL,
            target_status TEXT NOT NULL,
            expected_version INTEGER NOT NULL,
            preparation_minutes INTEGER NOT NULL,
            response_version INTEGER NOT NULL,
            created_epoch INTEGER NOT NULL,
            PRIMARY KEY(restaurant_id,idempotency_key)
        );
        CREATE TABLE IF NOT EXISTS restaurant_order_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            restaurant_id INTEGER NOT NULL,
            order_id INTEGER NOT NULL,
            actor_user_id INTEGER NOT NULL,
            from_status TEXT NOT NULL,
            to_status TEXT NOT NULL,
            preparation_minutes INTEGER NOT NULL,
            correlation_id TEXT NOT NULL,
            created_epoch INTEGER NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_restaurant_order_events_order
            ON restaurant_order_events(restaurant_id,order_id,id DESC);
    )sql";
    char* error = nullptr;
    m_ready = sqlite3_exec(m_database.connection(), schema, nullptr, nullptr, &error) == SQLITE_OK;
    if (!m_ready)
    {
        std::cerr << "[partner-order] schema failed error="
                  << (error ? error : "unknown") << std::endl;
        sqlite3_free(error);
    }
}

bool PartnerOrderRepository::ready() const
{
    return m_ready;
}

std::vector<PartnerOrderView> PartnerOrderRepository::listPaidOrders(
    int restaurantId,
    int limit)
{
    std::lock_guard<std::recursive_mutex> lock(m_database.mutex());
    std::vector<PartnerOrderView> result;
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT o.id,o.restaurant_id,o.total_amount,o.status,"
        "o.delivery_address,o.item_summary,w.status,w.preparation_minutes,"
        "w.version,w.updated_epoch FROM orders o LEFT JOIN "
        "restaurant_order_workflows w ON w.order_id=o.id "
        "WHERE o.restaurant_id=? AND o.status IN "
        "('CONFIRMED','ASSIGNED','PICKED_UP','ON_THE_WAY','ARRIVING','DELIVERED') "
        "ORDER BY o.id DESC LIMIT ?;";
    if (sqlite3_prepare_v2(m_database.connection(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return result;
    sqlite3_bind_int(statement, 1, restaurantId);
    sqlite3_bind_int(statement, 2, std::clamp(limit, 1, 100));
    while (sqlite3_step(statement) == SQLITE_ROW) result.push_back(readOrder(statement));
    sqlite3_finalize(statement);
    return result;
}

PartnerOrderWriteOutcome PartnerOrderRepository::transition(
    int restaurantId,
    int orderId,
    int actorUserId,
    const std::string& targetStatus,
    int expectedVersion,
    int preparationMinutes,
    const std::string& idempotencyKey,
    const std::string& correlationId)
{
    std::lock_guard<std::recursive_mutex> lock(m_database.mutex());
    sqlite3* connection = m_database.connection();
    if (sqlite3_exec(connection, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr) != SQLITE_OK)
        return {PartnerOrderWriteResult::StorageError, {}, "Could not start transaction"};

    sqlite3_stmt* statement = nullptr;
    const char* commandSql =
        "SELECT order_id,target_status,expected_version,preparation_minutes "
        "FROM restaurant_order_commands "
        "WHERE restaurant_id=? AND idempotency_key=?;";
    if (sqlite3_prepare_v2(connection, commandSql, -1, &statement, nullptr) != SQLITE_OK)
    {
        rollback(connection);
        return {PartnerOrderWriteResult::StorageError, {}, "Could not read command"};
    }
    sqlite3_bind_int(statement, 1, restaurantId);
    sqlite3_bind_text(statement, 2, idempotencyKey.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(statement) == SQLITE_ROW)
    {
        const bool same = sqlite3_column_int(statement, 0) == orderId &&
            textColumn(statement, 1) == targetStatus &&
            sqlite3_column_int(statement, 2) == expectedVersion &&
            sqlite3_column_int(statement, 3) == preparationMinutes;
        sqlite3_finalize(statement);
        auto order = findPaidOrder(connection, restaurantId, orderId);
        rollback(connection);
        return same
            ? PartnerOrderWriteOutcome{PartnerOrderWriteResult::AlreadyApplied, order, {}}
            : PartnerOrderWriteOutcome{PartnerOrderWriteResult::IdempotencyConflict, {},
                "Idempotency key was already used for a different command"};
    }
    sqlite3_finalize(statement);

    auto order = findPaidOrder(connection, restaurantId, orderId);
    if (!order)
    {
        rollback(connection);
        return {PartnerOrderWriteResult::NotFound, {}, "Paid order not found for this restaurant"};
    }
    if (order->version != expectedVersion)
    {
        rollback(connection);
        return {PartnerOrderWriteResult::VersionConflict, order,
            "Order changed; refresh before retrying"};
    }

    const auto current = restaurant_order::parseStatus(order->restaurantStatus);
    const auto target = restaurant_order::parseStatus(targetStatus);
    if (!current || !target)
    {
        rollback(connection);
        return {PartnerOrderWriteResult::InvalidTransition, order,
            "Unknown restaurant order status"};
    }
    const auto decision = restaurant_order::canTransition(
        *current, *target, restaurant_order::driverAssigned(order->orderStatus));
    if (!decision.allowed)
    {
        rollback(connection);
        return {PartnerOrderWriteResult::InvalidTransition, order, decision.message};
    }

    const int nextPreparation = targetStatus == "ACCEPTED"
        ? preparationMinutes : order->preparationMinutes;
    const int nextVersion = order->version + 1;
    const long long now = nowEpoch();
    const char* workflowSql =
        "INSERT INTO restaurant_order_workflows"
        "(order_id,restaurant_id,status,preparation_minutes,version,updated_epoch) "
        "VALUES(?,?,?,?,?,?) ON CONFLICT(order_id) DO UPDATE SET "
        "status=excluded.status,preparation_minutes=excluded.preparation_minutes,"
        "version=excluded.version,updated_epoch=excluded.updated_epoch;";
    if (sqlite3_prepare_v2(connection, workflowSql, -1, &statement, nullptr) != SQLITE_OK)
    {
        rollback(connection);
        return {PartnerOrderWriteResult::StorageError, {}, "Could not prepare workflow update"};
    }
    sqlite3_bind_int(statement, 1, orderId);
    sqlite3_bind_int(statement, 2, restaurantId);
    sqlite3_bind_text(statement, 3, targetStatus.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 4, nextPreparation);
    sqlite3_bind_int(statement, 5, nextVersion);
    sqlite3_bind_int64(statement, 6, now);
    const bool workflowUpdated = sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    if (!workflowUpdated)
    {
        rollback(connection);
        return {PartnerOrderWriteResult::StorageError, {}, "Could not update workflow"};
    }

    const char* insertCommand =
        "INSERT INTO restaurant_order_commands"
        "(restaurant_id,idempotency_key,order_id,target_status,expected_version,"
        "preparation_minutes,response_version,created_epoch) VALUES(?,?,?,?,?,?,?,?);";
    if (sqlite3_prepare_v2(connection, insertCommand, -1, &statement, nullptr) != SQLITE_OK)
    {
        rollback(connection);
        return {PartnerOrderWriteResult::StorageError, {}, "Could not store command"};
    }
    sqlite3_bind_int(statement, 1, restaurantId);
    sqlite3_bind_text(statement, 2, idempotencyKey.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 3, orderId);
    sqlite3_bind_text(statement, 4, targetStatus.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 5, expectedVersion);
    sqlite3_bind_int(statement, 6, preparationMinutes);
    sqlite3_bind_int(statement, 7, nextVersion);
    sqlite3_bind_int64(statement, 8, now);
    const bool commandStored = sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);

    const char* eventSql =
        "INSERT INTO restaurant_order_events"
        "(restaurant_id,order_id,actor_user_id,from_status,to_status,"
        "preparation_minutes,correlation_id,created_epoch) VALUES(?,?,?,?,?,?,?,?);";
    if (sqlite3_prepare_v2(connection, eventSql, -1, &statement, nullptr) != SQLITE_OK)
    {
        rollback(connection);
        return {PartnerOrderWriteResult::StorageError, {}, "Could not store audit event"};
    }
    sqlite3_bind_int(statement, 1, restaurantId);
    sqlite3_bind_int(statement, 2, orderId);
    sqlite3_bind_int(statement, 3, actorUserId);
    sqlite3_bind_text(statement, 4, order->restaurantStatus.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 5, targetStatus.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 6, nextPreparation);
    sqlite3_bind_text(statement, 7, correlationId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(statement, 8, now);
    const bool eventStored = sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);

    if (!commandStored || !eventStored ||
        sqlite3_exec(connection, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK)
    {
        rollback(connection);
        return {PartnerOrderWriteResult::StorageError, {}, "Could not commit workflow"};
    }

    auto updated = findPaidOrder(connection, restaurantId, orderId);
    std::clog << "[partner-order] transition restaurant=" << restaurantId
              << " order=" << orderId << " actor=" << actorUserId
              << " from=" << order->restaurantStatus << " to=" << targetStatus
              << " version=" << nextVersion << std::endl;
    return {PartnerOrderWriteResult::Updated, updated, {}};
}
