#pragma once

#include <string>
#include <vector>

class HttpClient
{
public:

    std::string get(
    const std::string& url,
    const std::string& authHeader = "");

std::string post(
    const std::string& url,
    const std::string& body,
    const std::string& authHeader = "",
    const std::vector<std::string>& extraHeaders = {});

std::string put(
    const std::string& url,
    const std::string& body,
    const std::string& authHeader = "");

std::string remove(
    const std::string& url,
    const std::string& authHeader = "");
};
