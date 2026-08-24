#include "RestaurantRepository.h"

#include <iostream>
#include <sqlite3.h>

RestaurantRepository::RestaurantRepository(Database& database)
    : m_database(database)
{
}

bool RestaurantRepository::saveRestaurant(const Restaurant& restaurant)
{
    const char* sql =
        "INSERT INTO restaurants(name,address,phone,rating,latitude,longitude,delivery_radius_km,image_url,delivery_polygon,base_delivery_fee,per_km_fee,preparation_minutes) "
        "VALUES(?,?,?,?,?,?,?,?,?,?,?,?);";

    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(
        m_database.connection(),
        sql,
        -1,
        &stmt,
        nullptr);

    if (rc != SQLITE_OK)
    {
        std::cerr << sqlite3_errmsg(m_database.connection()) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, restaurant.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, restaurant.getAddress().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, restaurant.getPhone().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, restaurant.getRating());
    sqlite3_bind_double(stmt, 5, restaurant.getLatitude());
    sqlite3_bind_double(stmt, 6, restaurant.getLongitude());
    sqlite3_bind_double(stmt, 7, restaurant.getDeliveryRadiusKm());
    sqlite3_bind_text(stmt, 8, restaurant.getImageUrl().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, restaurant.getDeliveryPolygon().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 10, restaurant.getBaseDeliveryFee()); sqlite3_bind_double(stmt, 11, restaurant.getPerKmFee()); sqlite3_bind_int(stmt, 12, restaurant.getPreparationMinutes());

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::vector<Restaurant> RestaurantRepository::getAllRestaurants()
{
    std::vector<Restaurant> restaurants;

    const char* sql =
        "SELECT id,name,address,phone,rating,latitude,longitude,delivery_radius_km,image_url,delivery_polygon,base_delivery_fee,per_km_fee,preparation_minutes FROM restaurants;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
            m_database.connection(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return restaurants;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        restaurants.emplace_back(
            sqlite3_column_int(stmt, 0),

            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 1)),

            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 2)),

            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 3)),

            sqlite3_column_double(stmt, 4),
            sqlite3_column_double(stmt, 5),
            sqlite3_column_double(stmt, 6),
            sqlite3_column_double(stmt, 7),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)), reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9)), sqlite3_column_double(stmt,10), sqlite3_column_double(stmt,11), sqlite3_column_int(stmt,12));
    }

    sqlite3_finalize(stmt);

    return restaurants;
}

std::optional<Restaurant> RestaurantRepository::getRestaurantById(int id)
{
    const char* sql =
        "SELECT id,name,address,phone,rating,latitude,longitude,delivery_radius_km,image_url,delivery_polygon,base_delivery_fee,per_km_fee,preparation_minutes "
        "FROM restaurants "
        "WHERE id=?;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
            m_database.connection(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        Restaurant restaurant(
            sqlite3_column_int(stmt, 0),

            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 1)),

            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 2)),

            reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 3)),

            sqlite3_column_double(stmt, 4),
            sqlite3_column_double(stmt, 5),
            sqlite3_column_double(stmt, 6),
            sqlite3_column_double(stmt, 7),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)), reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9)), sqlite3_column_double(stmt,10), sqlite3_column_double(stmt,11), sqlite3_column_int(stmt,12));

        sqlite3_finalize(stmt);

        return restaurant;
    }

    sqlite3_finalize(stmt);

    return std::nullopt;
}

bool RestaurantRepository::updateRestaurant(const Restaurant& restaurant)
{
    const char* sql =
        "UPDATE restaurants "
        "SET name=?,address=?,phone=?,rating=?,latitude=?,longitude=?,delivery_radius_km=?,image_url=?,delivery_polygon=?,base_delivery_fee=?,per_km_fee=?,preparation_minutes=? "
        "WHERE id=?;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
            m_database.connection(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, restaurant.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, restaurant.getAddress().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, restaurant.getPhone().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, restaurant.getRating());
    sqlite3_bind_double(stmt, 5, restaurant.getLatitude());
    sqlite3_bind_double(stmt, 6, restaurant.getLongitude());
    sqlite3_bind_double(stmt, 7, restaurant.getDeliveryRadiusKm());
    sqlite3_bind_text(stmt, 8, restaurant.getImageUrl().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,9,restaurant.getDeliveryPolygon().c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_double(stmt,10,restaurant.getBaseDeliveryFee()); sqlite3_bind_double(stmt,11,restaurant.getPerKmFee()); sqlite3_bind_int(stmt,12,restaurant.getPreparationMinutes());
    sqlite3_bind_int(stmt, 13, restaurant.getId());

    int rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        return false;
    }

    return sqlite3_changes(m_database.connection()) > 0;
}

bool RestaurantRepository::deleteRestaurant(int id)
{
    const char* sql =
        "DELETE FROM restaurants WHERE id=?;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
            m_database.connection(),
            sql,
            -1,
            &stmt,
            nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);

    int rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        return false;
    }

    return sqlite3_changes(m_database.connection()) > 0;
}
