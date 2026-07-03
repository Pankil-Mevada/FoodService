#include "Notification.h"

Notification::Notification(
    int id,
    int userId,
    const std::string& type,
    const std::string& message,
    const std::string& status,
    const std::string& createdAt)
    :
    m_id(id),
    m_userId(userId),
    m_type(type),
    m_message(message),
    m_status(status),
    m_createdAt(createdAt)
{
}

int Notification::getId() const
{
    return m_id;
}

int Notification::getUserId() const
{
    return m_userId;
}

const std::string& Notification::getType() const
{
    return m_type;
}

const std::string& Notification::getMessage() const
{
    return m_message;
}

const std::string& Notification::getStatus() const
{
    return m_status;
}

const std::string& Notification::getCreatedAt() const
{
    return m_createdAt;
}
