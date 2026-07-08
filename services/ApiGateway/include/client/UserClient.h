#pragma once

#include <string>

#include "client/HttpClient.h"

class UserClient
{
public:

    std::string registerUser(
        const std::string& jsonBody);

    std::string login(
        const std::string& jsonBody);

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

private:

    HttpClient m_httpClient;
};