std::string getAllUsers(
    const std::string& authHeader);

std::string getUserById(
    int id,
    const std::string& authHeader);

std::string updateUser(
    int id,
    const std::string& jsonBody,
    const std::string& authHeader);

std::string deleteUser(
    int id,
    const std::string& authHeader); 