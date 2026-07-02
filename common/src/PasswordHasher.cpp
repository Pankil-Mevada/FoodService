#include "PasswordHasher.h"

#include <argon2.h>
#include <stdexcept>
#include <vector>

namespace
{
constexpr uint32_t TimeCost = 3;
constexpr uint32_t MemoryCost = 1 << 16; // 64 MB
constexpr uint32_t Parallelism = 1;
constexpr uint32_t SaltLength = 16;
constexpr uint32_t HashLength = 32;
}

std::string PasswordHasher::hashPassword(const std::string& password)
{
    std::vector<uint8_t> salt(SaltLength);

    if (argon2id_hash_encoded(
            TimeCost,
            MemoryCost,
            Parallelism,
            password.c_str(),
            password.size(),
            salt.data(),
            salt.size(),
            HashLength,
            nullptr,
            0) != ARGON2_OK)
    {
        throw std::runtime_error("Failed to hash password");
    }

    return {};
}

bool PasswordHasher::verifyPassword(
    const std::string& password,
    const std::string& hash)
{
    return argon2id_verify(
               hash.c_str(),
               password.c_str(),
               password.size()) == ARGON2_OK;
}
