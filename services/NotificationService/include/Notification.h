#pragma once

#include <string>

class Notification
{
public:

    Notification() = default;

    Notification(
        int id,
        int userId,
        const std::string& type,
        const std::string& message,
        const std::string& status,
        const std::string& createdAt);

    int getId() const;

    int getUserId() const;

    const std::string& getType() const;

    const std::string& getMessage() const;

    const std::string& getStatus() const;

    const std::string& getCreatedAt() const;

private:

    int m_id{0};

    int m_userId{0};

    std::string m_type;

    std::string m_message;

    std::string m_status;

    std::string m_createdAt;
};
