#include "UserService.h"
#include "PasswordHasher.h"
#include <iostream>

UserService::UserService(UserRepository& repository)
    : m_repository(repository)
{
}

bool UserService::registerUser(const User& user)
{
	auto existingUser = m_repository.findByEmail(user.getEmail());

	if (existingUser.has_value())
	{
		return false;
	}
    try {
        User secured(user.getId(), user.getName(), user.getEmail(),
                     PasswordHasher::hashPassword(user.getPassword()));
        return m_repository.saveUser(secured);
    } catch (const std::exception&) {
        std::clog << "[ERROR] event=user.registration_failed reason=password_hashing" << std::endl;
        return false;
    }
}
std::vector<User> UserService::getAllUsers()
{
    return m_repository.getAllUsers();
}

std::optional<User> UserService::getUserById(int id)
{
    return m_repository.getUserById(id);
}

bool UserService::updateUser(const User& user)
{
    return m_repository.updateUser(user);
}

bool UserService::deleteUser(int id)
{
    return m_repository.deleteUser(id);
}

std::optional<std::string> UserService::login(
    const std::string& email,
    const std::string& password)
{
    auto user = m_repository.findByEmail(email);

    if (!user.has_value())
    {
        std::clog << "[WARNING] event=user.login_rejected reason=invalid_credentials" << std::endl;
        return std::nullopt;
    }

    const bool encoded = user->getPassword().rfind("$argon2id$", 0) == 0;
    const bool passwordMatches = encoded
        ? PasswordHasher::verifyPassword(password, user->getPassword())
        : user->getPassword() == password;
    if (!passwordMatches)
    {
        std::clog << "[WARNING] event=user.login_rejected reason=invalid_credentials" << std::endl;
        return std::nullopt;
    }

    if (!encoded) {
        try {
            m_repository.updatePasswordHash(user->getId(), PasswordHasher::hashPassword(password));
            std::clog << "[INFO] event=user.password_hash_upgraded userId=" << user->getId() << std::endl;
        } catch (const std::exception&) {
            std::clog << "[ERROR] event=user.password_hash_upgrade_failed userId=" << user->getId() << std::endl;
        }
    }

    JwtManager jwt;

    auto token = jwt.generateToken(
        user->getId(),
        user->getEmail());

    std::clog << "[INFO] event=user.login_succeeded userId=" << user->getId() << std::endl;

    return token;
}
