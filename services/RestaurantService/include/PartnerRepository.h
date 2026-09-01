#pragma once

#include "Database.h"
#include <optional>
#include <string>
#include <vector>

struct PartnerRestaurantRecord {
    int id{};
    std::string name;
    std::string address;
    std::string phone;
    double latitude{};
    double longitude{};
    double deliveryRadiusKm{8.0};
    std::string imageUrl;
    double baseDeliveryFee{39.0};
    double perKmFee{5.0};
    int preparationMinutes{20};
    std::string status{"DRAFT"};
    std::string role{"OWNER"};
    int version{1};
};

struct PartnerMenuItemRecord {
    int id{};
    int restaurantId{};
    std::string name;
    std::string description;
    long long pricePaise{};
    std::string dietType{"VEG"};
    bool available{true};
    int version{1};
};

struct PartnerAuditRecord {
    long long id{};
    int actorUserId{};
    std::string action;
    std::string resourceType;
    int resourceId{};
    std::string result;
    std::string correlationId;
    long long createdEpoch{};
};

enum class PartnerWriteResult { Ok, NotFound, Forbidden, Conflict, InvalidState, Invalid, Error };

class PartnerRepository {
public:
    explicit PartnerRepository(Database& database);
    bool ready() const { return m_ready; }

    PartnerWriteResult createRestaurant(int userId, const PartnerRestaurantRecord& value,
        const std::string& correlationId, int& restaurantId);
    std::vector<PartnerRestaurantRecord> listRestaurants(int userId);
    std::optional<PartnerRestaurantRecord> getRestaurant(int userId, int restaurantId);
    PartnerWriteResult updateRestaurant(int userId, PartnerRestaurantRecord& value,
        int expectedVersion, const std::string& correlationId);
    PartnerWriteResult submitRestaurant(int userId, int restaurantId, int expectedVersion,
        const std::string& correlationId, int& newVersion);

    std::vector<PartnerMenuItemRecord> listMenuItems(int userId, int restaurantId);
    PartnerWriteResult createMenuItem(int userId, PartnerMenuItemRecord& value,
        const std::string& correlationId);
    PartnerWriteResult updateMenuItem(int userId, PartnerMenuItemRecord& value,
        int expectedVersion, const std::string& correlationId);
    PartnerWriteResult deleteMenuItem(int userId, int restaurantId, int itemId,
        const std::string& correlationId);
    std::vector<PartnerAuditRecord> listAudit(int userId, int restaurantId, int limit);

private:
    bool ensureSchema();
    bool ensureRestaurantColumn(const std::string& name, const std::string& definition);
    std::optional<std::string> activeRole(int userId, int restaurantId);
    std::optional<std::string> restaurantStatus(int restaurantId);
    bool appendAudit(int actorUserId, int restaurantId, const std::string& action,
        const std::string& resourceType, int resourceId, const std::string& result,
        const std::string& correlationId);

    Database& m_database;
    bool m_ready{false};
};
