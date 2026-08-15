#include "Order.h"

Order::Order(
    int id,
    int userId,
    int restaurantId,
    double totalAmount,
    const std::string& status,
    double deliveryLatitude,
    double deliveryLongitude,
    const std::string& deliveryAddress)
    :
    m_id(id),
    m_userId(userId),
    m_restaurantId(restaurantId),
    m_totalAmount(totalAmount),
    m_status(status),
    m_deliveryLatitude(deliveryLatitude),
    m_deliveryLongitude(deliveryLongitude),
    m_deliveryAddress(deliveryAddress)
{
}

int Order::getId() const
{
    return m_id;
}

int Order::getUserId() const
{
    return m_userId;
}

int Order::getRestaurantId() const
{
    return m_restaurantId;
}

double Order::getTotalAmount() const
{
    return m_totalAmount;
}

const std::string& Order::getStatus() const
{
    return m_status;
}

double Order::getDeliveryLatitude() const { return m_deliveryLatitude; }
double Order::getDeliveryLongitude() const { return m_deliveryLongitude; }
const std::string& Order::getDeliveryAddress() const { return m_deliveryAddress; }
