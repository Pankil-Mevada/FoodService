#pragma once

#include <string>

class JwtManager
{
public:

    JwtManager();

    std::string generateToken(
        int userId,
        const std::string& email);

    bool verifyToken(
        const std::string& token);

    int getUserId(
        const std::string& token);

    std::string getEmail(
        const std::string& token);

private:

    std::string m_secret;
};
