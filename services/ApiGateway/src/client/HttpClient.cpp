#include "client/HttpClient.h"
#include "CorrelationId.h"

#include <curl/curl.h>

namespace
{
constexpr long connectTimeoutMs = 2000;
constexpr long requestTimeoutMs = 10000;

size_t writeCallback(void* contents, size_t size, size_t count, void* target)
{
    static_cast<std::string*>(target)->append(
        static_cast<char*>(contents), size * count);
    return size * count;
}

TransportFailure classifyFailure(CURLcode code)
{
    if (code == CURLE_OPERATION_TIMEDOUT) return TransportFailure::Timeout;
    if (code == CURLE_COULDNT_CONNECT || code == CURLE_COULDNT_RESOLVE_HOST ||
        code == CURLE_COULDNT_RESOLVE_PROXY)
        return TransportFailure::Connection;
    return code == CURLE_OK ? TransportFailure::None : TransportFailure::Other;
}

HttpResult perform(const std::string& method, const std::string& url,
    const std::string& body, const std::string& authHeader,
    const std::vector<std::string>& extraHeaders)
{
    HttpResult result;
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        result.failure = TransportFailure::Other;
        result.error = "Could not initialize HTTP client";
        return result;
    }

    curl_slist* headers = nullptr;
    if (method == "POST" || method == "PUT")
        headers = curl_slist_append(headers, "Content-Type: application/json");
    if (!authHeader.empty())
    {
        const std::string value = "Authorization: " + authHeader;
        headers = curl_slist_append(headers, value.c_str());
    }
    for (const auto& value : extraHeaders)
        headers = curl_slist_append(headers, value.c_str());
    if (!currentCorrelationId.empty())
    {
        const std::string value = "X-Correlation-ID: " + currentCorrelationId;
        headers = curl_slist_append(headers, value.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT,
        "FoodService-local-development/0.1 (https://github.com/Pankil-Mevada/FoodService)");
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, connectTimeoutMs);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, requestTimeoutMs);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.body);
    if (headers) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (method == "POST")
    {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    }
    else if (method == "PUT")
    {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    }
    else if (method == "DELETE")
    {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }

    const CURLcode code = curl_easy_perform(curl);
    result.failure = classifyFailure(code);
    if (code != CURLE_OK) result.error = curl_easy_strerror(code);
    if (code == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.statusCode);

    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return result;
}
}

HttpResult HttpClient::get(const std::string& url, const std::string& authHeader)
{
    return perform("GET", url, "", authHeader, {});
}

HttpResult HttpClient::post(const std::string& url, const std::string& body,
    const std::string& authHeader, const std::vector<std::string>& extraHeaders)
{
    return perform("POST", url, body, authHeader, extraHeaders);
}

HttpResult HttpClient::put(const std::string& url, const std::string& body,
    const std::string& authHeader)
{
    return perform("PUT", url, body, authHeader, {});
}

HttpResult HttpClient::remove(const std::string& url, const std::string& authHeader)
{
    return perform("DELETE", url, "", authHeader, {});
}
