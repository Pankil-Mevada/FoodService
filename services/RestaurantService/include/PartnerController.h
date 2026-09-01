#pragma once

#include "PartnerRepository.h"
#include <crow.h>

class PartnerController {
public:
    explicit PartnerController(PartnerRepository& repository);

    crow::response listRestaurants(const crow::request& request);
    crow::response createRestaurant(const crow::request& request);
    crow::response getRestaurant(const crow::request& request, int restaurantId);
    crow::response updateRestaurant(const crow::request& request, int restaurantId);
    crow::response submitRestaurant(const crow::request& request, int restaurantId);
    crow::response listMenuItems(const crow::request& request, int restaurantId);
    crow::response createMenuItem(const crow::request& request, int restaurantId);
    crow::response updateMenuItem(const crow::request& request, int restaurantId, int itemId);
    crow::response deleteMenuItem(const crow::request& request, int restaurantId, int itemId);
    crow::response listAudit(const crow::request& request, int restaurantId);

private:
    PartnerRepository& m_repository;
};
