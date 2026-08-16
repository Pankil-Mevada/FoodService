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
        const std::string& status,
        double deliveryLatitude = 0.0,
        double deliveryLongitude = 0.0,
        const std::string& deliveryAddress = "",
        const std::string& itemSummary = "",
        double subtotal = 0.0,
        double discountAmount = 0.0,
        double deliveryFee = 0.0);

    int getId() const;

    int getUserId() const;

    int getRestaurantId() const;

    double getTotalAmount() const;

    const std::string& getStatus() const;
    double getDeliveryLatitude() const;
    double getDeliveryLongitude() const;
    const std::string& getDeliveryAddress() const;
    const std::string& getItemSummary() const;
    double getSubtotal() const;
    double getDiscountAmount() const;
    double getDeliveryFee() const;

private:

    int m_id{0};

    int m_userId{0};

    int m_restaurantId{0};

    double m_totalAmount{0.0};

    std::string m_status;
    double m_deliveryLatitude{0.0};
    double m_deliveryLongitude{0.0};
    std::string m_deliveryAddress;
    std::string m_itemSummary;
    double m_subtotal{0.0};
    double m_discountAmount{0.0};
    double m_deliveryFee{0.0};
};
