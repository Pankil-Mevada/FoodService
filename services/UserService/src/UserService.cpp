#include "UserService.h"

UserService::UserService(UserRepository& repository)
    : m_repository(repository)
{
}

bool UserService::registerUser(const User& user)
{
    return m_repository.saveUser(user);
}
