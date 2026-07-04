#pragma once

#include <string>

class NotificationClient
{
public:

    bool createNotification(
        int userId,
        const std::string& message);
};
