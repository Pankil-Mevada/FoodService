#pragma once

#include <string>

class User
{
public:

    User() = default;

    User(
        int id,
        const std::string& name,
        const std::string& email,
        const std::string& password);

    int getId() const;

    const std::string& getName() const;

    const std::string& getEmail() const;

    const std::string& getPassword() const;

private:

    int m_id{0};

    std::string m_name;

    std::string m_email;

    std::string m_password;
};
