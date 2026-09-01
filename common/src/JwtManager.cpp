#include "JwtManager.h"

#include <chrono>
#include <cstdlib>
#include <stdexcept>

#include <jwt-cpp/traits/kazuho-picojson/defaults.h>

JwtManager::JwtManager()
{
    const char* configured=std::getenv("FOODSERVICE_JWT_SECRET");
    const char* environment=std::getenv("FOODSERVICE_ENV");
    const bool production=environment&&std::string(environment)=="production";
    if(configured&&std::string(configured).size()>=32) m_secret=configured;
    else if(production) throw std::runtime_error(
        "FOODSERVICE_JWT_SECRET must contain at least 32 characters in production");
    else m_secret="FoodService-Local-Development-Secret-Only";
}

std::string JwtManager::generateToken(int userId,const std::string& email)
{
    using namespace std::chrono;
    return jwt::create()
        .set_issuer("FoodService")
        .set_audience("FoodServiceWeb")
        .set_subject(email)
        .set_payload_claim("userId",jwt::claim(std::to_string(userId)))
        .set_payload_claim("tokenVersion",jwt::claim(std::string("1")))
        .set_issued_at(system_clock::now())
        .set_expires_at(system_clock::now()+hours{1})
        .sign(jwt::algorithm::hs256{m_secret});
}

bool JwtManager::verifyToken(const std::string& token)
{
    try {
        const auto decoded=jwt::decode(token);
        auto verifier=jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{m_secret})
            .with_issuer("FoodService")
            .with_audience("FoodServiceWeb");
        verifier.verify(decoded);
        return decoded.has_payload_claim("userId")&&
            decoded.has_payload_claim("tokenVersion")&&
            decoded.get_payload_claim("tokenVersion").as_string()=="1";
    } catch(const std::exception&) { return false; }
}

int JwtManager::getUserId(const std::string& token)
{
    return std::stoi(jwt::decode(token).get_payload_claim("userId").as_string());
}

std::string JwtManager::getEmail(const std::string& token)
{
    return jwt::decode(token).get_subject();
}
