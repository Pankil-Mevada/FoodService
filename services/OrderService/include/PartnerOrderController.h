#pragma once

#include <crow.h>

#include "PartnerOrderRepository.h"

class PartnerOrderController
{
public:
    explicit PartnerOrderController(PartnerOrderRepository& repository);

    crow::response listOrders(const crow::request& req, int restaurantId);
    crow::response transitionOrder(
        const crow::request& req,
        int restaurantId,
        int orderId);

private:
    PartnerOrderRepository& m_repository;
};
