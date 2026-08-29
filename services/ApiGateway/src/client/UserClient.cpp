#include "client/UserClient.h"

HttpResult UserClient::registerUser(
    const std::string& jsonBody)
{
    return m_httpClient.post(
        "http://localhost:8080/register",
        jsonBody);
}

HttpResult UserClient::login(
    const std::string& jsonBody)
{
    return m_httpClient.post(
        "http://localhost:8080/login",
        jsonBody);
}

HttpResult UserClient::getAllUsers(
    const std::string& authHeader)
{
    return m_httpClient.get(
        "http://localhost:8080/users",
        authHeader);
}

HttpResult UserClient::getUserById(
    int id,
    const std::string& authHeader)
{
    return m_httpClient.get(
        "http://localhost:8080/users/" +
        std::to_string(id),
        authHeader);
}

HttpResult UserClient::updateUser(
    int id,
    const std::string& jsonBody,
    const std::string& authHeader)
{
    return m_httpClient.put(
        "http://localhost:8080/users/" +
        std::to_string(id),
        jsonBody,
        authHeader);
}

HttpResult UserClient::deleteUser(
    int id,
    const std::string& authHeader)
{
    return m_httpClient.remove(
        "http://localhost:8080/users/" +
        std::to_string(id),
        authHeader);
}
