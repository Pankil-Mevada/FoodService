#include "UserValidator.h"
#include <algorithm>
#include <cctype>

bool UserValidator::validateName(const std::string& name)
{
    return !name.empty() && name.length() >= 3;
}

bool UserValidator::validateEmail(const std::string& email)
{
    if (email.empty() || email.size() > 254 || email.find(' ') != std::string::npos)
        return false;
    auto atPos = email.find('@');

    auto dotPos = email.rfind('.');

    return atPos != std::string::npos &&
           dotPos != std::string::npos &&
           atPos > 0 && atPos < dotPos &&
           email.find('@', atPos + 1) == std::string::npos &&
           dotPos > atPos + 1 &&
           dotPos != email.length() - 1;
}

bool UserValidator::validatePassword(const std::string& password)
{
    if (password.length() < 10 || password.length() > 128) return false;
    const auto has = [&password](auto predicate) {
        return std::any_of(password.begin(), password.end(), predicate);
    };
    return has([](unsigned char c) { return std::islower(c); }) &&
           has([](unsigned char c) { return std::isupper(c); }) &&
           has([](unsigned char c) { return std::isdigit(c); }) &&
           has([](unsigned char c) { return std::ispunct(c); });
}
