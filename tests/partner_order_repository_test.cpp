#include "Database.h"
#include "PartnerOrderRepository.h"

#include <cassert>
#include <filesystem>
#include <sqlite3.h>
#include <string>
#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

namespace
{
void insertOrder(sqlite3* database, int userId, int restaurantId, const char* status)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql = "INSERT INTO orders(user_id,restaurant_id,total_amount,status,delivery_address,item_summary) VALUES(?,?,?,?,?,?);";
    assert(sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) == SQLITE_OK);
    sqlite3_bind_int(statement, 1, userId);
    sqlite3_bind_int(statement, 2, restaurantId);
    sqlite3_bind_double(statement, 3, 499.0);
    sqlite3_bind_text(statement, 4, status, -1, SQLITE_STATIC);
    sqlite3_bind_text(statement, 5, "Test address", -1, SQLITE_STATIC);
    sqlite3_bind_text(statement, 6, "Test meal x 1", -1, SQLITE_STATIC);
    assert(sqlite3_step(statement) == SQLITE_DONE);
    sqlite3_finalize(statement);
}
}

int main()
{
    const auto path = (std::filesystem::temp_directory_path() /
        ("foodservice-partner-orders-" + std::to_string(getpid()) + ".db")).string();
    std::filesystem::remove(path);
    {
        Database database(path);
        database.createOrderTable();
        PartnerOrderRepository repository(database);
        assert(repository.ready());
        insertOrder(database.connection(), 1, 10, "PAYMENT_PENDING");
        insertOrder(database.connection(), 2, 10, "CONFIRMED");
        insertOrder(database.connection(), 3, 20, "CONFIRMED");

        auto orders = repository.listPaidOrders(10);
        assert(orders.size() == 1);
        assert(orders[0].restaurantStatus == "NEW");
        const int orderId = orders[0].id;

        auto foreign = repository.transition(20, orderId, 99, "ACCEPTED", 0, 25,
            "foreign-command", "test-foreign");
        assert(foreign.result == PartnerOrderWriteResult::NotFound);

        auto accepted = repository.transition(10, orderId, 7, "ACCEPTED", 0, 25,
            "accept-command", "test-accept");
        assert(accepted.result == PartnerOrderWriteResult::Updated);
        assert(accepted.order->version == 1);
        assert(accepted.order->preparationMinutes == 25);

        auto replay = repository.transition(10, orderId, 7, "ACCEPTED", 0, 25,
            "accept-command", "test-replay");
        assert(replay.result == PartnerOrderWriteResult::AlreadyApplied);
        auto changedPayload = repository.transition(10, orderId, 7, "ACCEPTED", 0, 30,
            "accept-command", "test-changed-payload");
        assert(changedPayload.result == PartnerOrderWriteResult::IdempotencyConflict);
        auto reused = repository.transition(10, orderId, 7, "PREPARING", 1, 25,
            "accept-command", "test-reuse");
        assert(reused.result == PartnerOrderWriteResult::IdempotencyConflict);
        auto stale = repository.transition(10, orderId, 7, "PREPARING", 0, 25,
            "stale-command", "test-stale");
        assert(stale.result == PartnerOrderWriteResult::VersionConflict);
        auto skipped = repository.transition(10, orderId, 7, "READY_FOR_PICKUP", 1, 25,
            "skip-command", "test-skip");
        assert(skipped.result == PartnerOrderWriteResult::InvalidTransition);

        auto preparing = repository.transition(10, orderId, 7, "PREPARING", 1, 25,
            "prepare-command", "test-prepare");
        assert(preparing.result == PartnerOrderWriteResult::Updated);
        auto ready = repository.transition(10, orderId, 7, "READY_FOR_PICKUP", 2, 25,
            "ready-command", "test-ready");
        assert(ready.result == PartnerOrderWriteResult::Updated);
        auto earlyHandoff = repository.transition(10, orderId, 7, "HANDED_OFF", 3, 25,
            "handoff-early", "test-handoff-early");
        assert(earlyHandoff.result == PartnerOrderWriteResult::InvalidTransition);

        assert(sqlite3_exec(database.connection(),
            ("UPDATE orders SET status='ASSIGNED' WHERE id=" + std::to_string(orderId)).c_str(),
            nullptr, nullptr, nullptr) == SQLITE_OK);
        auto handoff = repository.transition(10, orderId, 7, "HANDED_OFF", 3, 25,
            "handoff-command", "test-handoff");
        assert(handoff.result == PartnerOrderWriteResult::Updated);
        assert(handoff.order->restaurantStatus == "HANDED_OFF");

        sqlite3_stmt* statement = nullptr;
        assert(sqlite3_prepare_v2(database.connection(),
            "SELECT COUNT(*) FROM restaurant_order_events WHERE order_id=?;",
            -1, &statement, nullptr) == SQLITE_OK);
        sqlite3_bind_int(statement, 1, orderId);
        assert(sqlite3_step(statement) == SQLITE_ROW);
        assert(sqlite3_column_int(statement, 0) == 4);
        sqlite3_finalize(statement);
    }
    std::filesystem::remove(path);
}
