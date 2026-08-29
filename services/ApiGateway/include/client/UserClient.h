#pragma once

#include <string>

#include "client/HttpClient.h"

class UserClient
{
public:

    HttpResult registerUser(
        const std::string& jsonBody);

    HttpResult login(
        const std::string& jsonBody);

    HttpResult getAllUsers(
        const std::string& authHeader);

    HttpResult getUserById(
        int id,
        const std::string& authHeader);

    HttpResult updateUser(
        int id,
        const std::string& jsonBody,
        const std::string& authHeader);

    HttpResult deleteUser(
        int id,
        const std::string& authHeader);

private:

    HttpClient m_httpClient;
};
