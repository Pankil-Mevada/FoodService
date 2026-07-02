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

bool UserService::login(
    const std::string& email,
    const std::string& password)
{
    auto user = m_repository.findByEmail(email);

    if (!user.has_value())
    {
        return false;
    }

    if (user->getPassword() != password)
    {
        return false;
    }

    return true;
}
