#pragma once

#include <string>
#include <vector>

enum class TransportFailure { None, Timeout, Connection, Other };

struct HttpResult
{
    long statusCode{0};
    std::string body;
    TransportFailure failure{TransportFailure::None};
    std::string error;

    [[nodiscard]] bool transportSucceeded() const
    {
        return failure == TransportFailure::None;
    }
};

inline int gatewayStatusFor(const HttpResult& result)
{
    if (result.failure == TransportFailure::Timeout) return 504;
    if (result.failure != TransportFailure::None) return 502;
    if (result.statusCode >= 100 && result.statusCode <= 599)
        return static_cast<int>(result.statusCode);
    return 502;
}

class HttpClient
{
public:
    HttpResult get(const std::string& url, const std::string& authHeader = "",
        const std::vector<std::string>& extraHeaders = {});
    HttpResult post(const std::string& url, const std::string& body,
        const std::string& authHeader = "",
        const std::vector<std::string>& extraHeaders = {});
    HttpResult put(const std::string& url, const std::string& body,
        const std::string& authHeader = "");
    HttpResult remove(const std::string& url, const std::string& authHeader = "");
};
