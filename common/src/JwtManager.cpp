#include "JwtManager.h"

#include <chrono>
#include <iostream>

#include <jwt-cpp/traits/kazuho-picojson/defaults.h>

JwtManager::JwtManager()
    : m_secret("FoodServiceSecretKey")
{
}

std::string JwtManager::generateToken(
    int userId,
    const std::string& email)
{
    using namespace std::chrono;

    auto token =
        jwt::create()
            .set_issuer("FoodService")
            .set_subject(email)
            .set_payload_claim(
                "userId",
                jwt::claim(std::to_string(userId)))
            .set_issued_at(system_clock::now())
            .set_expires_at(system_clock::now() + hours{24})
            .sign(jwt::algorithm::hs256{m_secret});

    return token;
}

bool JwtManager::verifyToken(
    const std::string& token)
{
    try
    {
        auto decoded = jwt::decode(token);

        auto verifier =
            jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{m_secret})
                .with_issuer("FoodService");

        verifier.verify(decoded);

        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

int JwtManager::getUserId(
    const std::string& token)
{
    auto decoded = jwt::decode(token);

    return std::stoi(
        decoded.get_payload_claim("userId")
            .as_string());
}

std::string JwtManager::getEmail(
    const std::string& token)
{
    auto decoded = jwt::decode(token);

    return decoded.get_subject();
}
