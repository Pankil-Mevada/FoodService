#pragma once

#include <string>

class Order
{
public:

    Order() = default;

    Order(
        int id,
        int userId,
        int restaurantId,
        double totalAmount,
        const std::string& status);

    int getId() const;

    int getUserId() const;

    int getRestaurantId() const;

    double getTotalAmount() const;

    const std::string& getStatus() const;

private:

    int m_id{0};

    int m_userId{0};

    int m_restaurantId{0};

    double m_totalAmount{0.0};

    std::string m_status;
};
