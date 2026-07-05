#include "client/UserClient.h"

std::string UserClient::registerUser(
    const std::string& jsonBody)
{
    return m_httpClient.post(
        "http://localhost:8080/register",
        jsonBody);
}

std::string UserClient::login(
    const std::string& jsonBody)
{
    return m_httpClient.post(
        "http://localhost:8080/login",
        jsonBody);
}

std::string UserClient::getAllUsers(
    const std::string& authHeader)
{
    return m_httpClient.get(
        "http://localhost:8080/users",
        authHeader);
}

std::string UserClient::getUserById(
    int id,
    const std::string& authHeader)
{
    return m_httpClient.get(
        "http://localhost:8080/users/" +
        std::to_string(id),
        authHeader);
}

std::string UserClient::updateUser(
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

std::string UserClient::deleteUser(
    int id,
    const std::string& authHeader)
{
    return m_httpClient.remove(
        "http://localhost:8080/users/" +
        std::to_string(id),
        authHeader);
}