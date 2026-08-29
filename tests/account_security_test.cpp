#include "PasswordHasher.h"
#include "UserValidator.h"

#include <cassert>

int main()
{
    assert(UserValidator::validateEmail("customer@example.com"));
    assert(!UserValidator::validateEmail("customer example.com"));
    assert(!UserValidator::validateEmail("customer@@example.com"));
    assert(UserValidator::validatePassword("StrongTest!42"));
    assert(!UserValidator::validatePassword("abcdef"));
    assert(!UserValidator::validatePassword("alllowercase42!"));

    const auto first = PasswordHasher::hashPassword("StrongTest!42");
    const auto second = PasswordHasher::hashPassword("StrongTest!42");
    assert(first.rfind("$argon2id$", 0) == 0);
    assert(first != second);
    assert(PasswordHasher::verifyPassword("StrongTest!42", first));
    assert(!PasswordHasher::verifyPassword("WrongTest!42", first));
}
