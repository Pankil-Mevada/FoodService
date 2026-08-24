#pragma once
#include<string>

class Restaurant
{
public:


    Restaurant(
        int id,
        const std::string& name,
        const std::string& address,
        const std::string& phone,
        double rating,
        double latitude = 23.0225,
        double longitude = 72.5714,
        double deliveryRadiusKm = 8.0,
        const std::string& imageUrl = "",
        const std::string& deliveryPolygon = "",
        double baseDeliveryFee = 39.0,
        double perKmFee = 5.0,
        int preparationMinutes = 20);

    int getId() const;

    const std::string& getName() const;

    const std::string& getAddress() const;

    const std::string& getPhone() const;

    double getRating() const;
    double getLatitude() const;
    double getLongitude() const;
    double getDeliveryRadiusKm() const;
    const std::string& getImageUrl() const;
    const std::string& getDeliveryPolygon() const;
    double getBaseDeliveryFee() const;
    double getPerKmFee() const;
    int getPreparationMinutes() const;

private:

    int m_id{0};

    std::string m_name;

    std::string m_address;

    std::string m_phone;

    double m_rating{0.0};
    double m_latitude{23.0225};
    double m_longitude{72.5714};
    double m_deliveryRadiusKm{8.0};
    std::string m_imageUrl;
    std::string m_deliveryPolygon;
    double m_baseDeliveryFee{39.0};
    double m_perKmFee{5.0};
    int m_preparationMinutes{20};
};
