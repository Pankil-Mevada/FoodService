#include "OrderService.h"

OrderService::OrderService(OrderRepository& repository)
    : m_repository(repository)
{
}


bool OrderService::createOrder(const Order& order)
{
	bool restaurantFound =
		m_restaurantClient.restaurantExists(
				order.getRestaurantId());

	if (!restaurantFound)
	{
		return false;
	}

	bool status = m_repository.saveOrder(order);

	if (!status)
	{
		return false;
    }

    bool paymentStatus =
        m_paymentClient.createPayment(
            order.getId(),
            order.getTotalAmount());

    if (!paymentStatus)
    {
        // Later we'll update the order status to PAYMENT_FAILED.
        return false;
    }

    return true;
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
