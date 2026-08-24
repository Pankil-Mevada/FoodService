#include "Restaurant.h"

Restaurant::Restaurant(
    int id,
    const std::string& name,
    const std::string& address,
    const std::string& phone,
    double rating,
    double latitude,
    double longitude,
    double deliveryRadiusKm,
    const std::string& imageUrl,
    const std::string& deliveryPolygon,
    double baseDeliveryFee,
    double perKmFee,
    int preparationMinutes)
    :
    m_id(id),
    m_name(name),
    m_address(address),
    m_phone(phone),
    m_rating(rating),
    m_latitude(latitude),
    m_longitude(longitude),
    m_deliveryRadiusKm(deliveryRadiusKm),
    m_imageUrl(imageUrl), m_deliveryPolygon(deliveryPolygon), m_baseDeliveryFee(baseDeliveryFee),
    m_perKmFee(perKmFee), m_preparationMinutes(preparationMinutes)
{
}

int Restaurant::getId() const
{
    return m_id;
}

const std::string& Restaurant::getName() const
{
    return m_name;
}

const std::string& Restaurant::getAddress() const
{
    return m_address;
}

const std::string& Restaurant::getPhone() const
{
    return m_phone;
}

double Restaurant::getRating() const
{
    return m_rating;
}

double Restaurant::getLatitude() const { return m_latitude; }
double Restaurant::getLongitude() const { return m_longitude; }
double Restaurant::getDeliveryRadiusKm() const { return m_deliveryRadiusKm; }
const std::string& Restaurant::getImageUrl() const { return m_imageUrl; }
const std::string& Restaurant::getDeliveryPolygon() const { return m_deliveryPolygon; }
double Restaurant::getBaseDeliveryFee() const { return m_baseDeliveryFee; }
double Restaurant::getPerKmFee() const { return m_perKmFee; }
int Restaurant::getPreparationMinutes() const { return m_preparationMinutes; }
