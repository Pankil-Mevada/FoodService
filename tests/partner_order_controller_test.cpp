#include "Database.h"
#include "PartnerOrderController.h"
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
void insertPaidOrder(sqlite3* database)
{
    assert(sqlite3_exec(database,
        "INSERT INTO orders(user_id,restaurant_id,total_amount,status,delivery_address,item_summary) "
        "VALUES(1,10,299.0,'CONFIRMED','Test address','Test meal x 1');",
        nullptr, nullptr, nullptr) == SQLITE_OK);
}

crow::request partnerRequest(std::string body = {})
{
    crow::request request;
    request.body = std::move(body);
    request.add_header("X-Partner-User-ID", "7");
    return request;
}
}

int main()
{
    const auto path = (std::filesystem::temp_directory_path() /
        ("foodservice-partner-order-controller-" + std::to_string(getpid()) + ".db")).string();
    std::filesystem::remove(path);
    {
        Database database(path);
        database.createOrderTable();
        PartnerOrderRepository repository(database);
        PartnerOrderController controller(repository);
        assert(repository.ready());

        crow::request anonymous;
        assert(controller.listOrders(anonymous, 10).code == 401);
        auto empty = controller.listOrders(partnerRequest(), 10);
        assert(empty.code == 200);
        assert(empty.body == "[]");

        insertPaidOrder(database.connection());
        auto listed = controller.listOrders(partnerRequest(), 10);
        assert(listed.code == 200);
        auto listJson = crow::json::load(listed.body);
        assert(listJson && listJson.t() == crow::json::type::List && listJson.size() == 1);

        auto missingKey = partnerRequest("{\"status\":\"ACCEPTED\",\"expectedVersion\":0}");
        assert(controller.transitionOrder(missingKey, 10, 1).code == 400);

        auto wrongTypes = partnerRequest("{\"status\":1,\"expectedVersion\":\"zero\"}");
        wrongTypes.add_header("Idempotency-Key", "wrong-types-key");
        assert(controller.transitionOrder(wrongTypes, 10, 1).code == 400);

        auto negativeVersion = partnerRequest("{\"status\":\"ACCEPTED\",\"expectedVersion\":-1}");
        negativeVersion.add_header("Idempotency-Key", "negative-version-key");
        assert(controller.transitionOrder(negativeVersion, 10, 1).code == 422);

        auto accepted = partnerRequest("{\"status\":\"ACCEPTED\",\"expectedVersion\":0,\"preparationMinutes\":25}");
        accepted.add_header("Idempotency-Key", "accept-order-key");
        auto acceptedResponse = controller.transitionOrder(accepted, 10, 1);
        assert(acceptedResponse.code == 200);

        auto replay = partnerRequest("{\"status\":\"ACCEPTED\",\"expectedVersion\":0,\"preparationMinutes\":25}");
        replay.add_header("Idempotency-Key", "accept-order-key");
        auto replayResponse = controller.transitionOrder(replay, 10, 1);
        assert(replayResponse.code == 200);
        auto replayJson = crow::json::load(replayResponse.body);
        assert(replayJson && replayJson["idempotentReplay"].b());

        auto changedPayload = partnerRequest("{\"status\":\"ACCEPTED\",\"expectedVersion\":0,\"preparationMinutes\":30}");
        changedPayload.add_header("Idempotency-Key", "accept-order-key");
        assert(controller.transitionOrder(changedPayload, 10, 1).code == 409);
    }
    std::filesystem::remove(path);
}
