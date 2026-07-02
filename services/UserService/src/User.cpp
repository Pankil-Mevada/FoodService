#include "User.h"

User::User(const std::string& name,
           const std::string& email,
           const std::string& password)
    : name(name),
      email(email),
      password(password)
{
}
