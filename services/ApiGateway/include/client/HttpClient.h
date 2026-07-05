#pragma once

#include <string>

class HttpClient
{
public:

    std::string get(
        const std::string& url);

    std::string post(
        const std::string& url,
        const std::string& body);

    std::string put(
        const std::string& url,
        const std::string& body);

    std::string remove(
        const std::string& url);
};