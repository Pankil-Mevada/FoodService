#include "OrderService.h"

OrderService::OrderService(OrderRepository& repository)
    : m_repository(repository)
{
}

bool OrderService::createOrder(const Order& order)
{
    return m_repository.saveOrder(order);
}

std::vector<Order> OrderService::getAllOrders()
{
    return m_repository.getAllOrders();
}

std::optional<Order> OrderService::getOrderById(int id)
{
    return m_repository.getOrderById(id);
}

bool OrderService::updateOrder(const Order& order)
{
    return m_repository.updateOrder(order);
}

bool OrderService::deleteOrder(int id)
{
    return m_repository.deleteOrder(id);
}
