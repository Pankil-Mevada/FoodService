#include "UserService.h"

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
    return m_repository.saveUser(user);
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
    std::cout << "Login email    : " << email << std::endl;
    std::cout << "Login password : " << password << std::endl;

    auto user = m_repository.findByEmail(email);

    if (!user.has_value())
    {
        std::cout << "User not found" << std::endl;
        return std::nullopt;
    }

    std::cout << "DB email       : " << user->getEmail() << std::endl;
    std::cout << "DB password    : " << user->getPassword() << std::endl;

    if (user->getPassword() != password)
    {
        std::cout << "Password mismatch" << std::endl;
        return std::nullopt;
    }

    std::cout << "Password matched" << std::endl;

    JwtManager jwt;

    auto token = jwt.generateToken(
        user->getId(),
        user->getEmail());

    std::cout << "JWT generated" << std::endl;

    return token;
}
