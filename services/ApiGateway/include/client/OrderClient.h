#pragma once

#include <string>

class OrderClient
{
public:
    std::string createOrder(const std::string& jsonBody);
};