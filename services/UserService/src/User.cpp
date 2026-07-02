#include "User.h"

User::User(
    int id,
    const std::string& name,
    const std::string& email,
    const std::string& password)
    :
    m_id(id),
    m_name(name),
    m_email(email),
    m_password(password)
{
}

int User::getId() const
{
    return m_id;
}

const std::string& User::getName() const
{
    return m_name;
}

const std::string& User::getEmail() const
{
    return m_email;
}

const std::string& User::getPassword() const
{
    return m_password;
}
