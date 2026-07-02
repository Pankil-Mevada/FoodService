#include "Restaurant.h"

Restaurant::Restaurant(
    int id,
    const std::string& name,
    const std::string& address,
    const std::string& phone,
    double rating)
    :
    m_id(id),
    m_name(name),
    m_address(address),
    m_phone(phone),
    m_rating(rating)
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
