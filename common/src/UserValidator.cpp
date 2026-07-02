#include "UserValidator.h"

bool UserValidator::validateName(const std::string& name)
{
    return !name.empty() && name.length() >= 3;
}

bool UserValidator::validateEmail(const std::string& email)
{
    auto atPos = email.find('@');

    auto dotPos = email.rfind('.');

    return atPos != std::string::npos &&
           dotPos != std::string::npos &&
           atPos < dotPos &&
           dotPos != email.length() - 1;
}

bool UserValidator::validatePassword(const std::string& password)
{
    return password.length() >= 6;
}
