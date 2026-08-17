#include "client/PaymentClient.h"
#include <crow.h>
#include <iostream>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>

namespace
{
size_t paymentResponseCallback(void* data, size_t size, size_t count, void* target)
{
    static_cast<std::string*>(target)->append(static_cast<char*>(data), size * count);
    return size * count;
}
}

bool PaymentClient::createPayment(
    int orderId,
    int userId,
    double amount)
{
    CURL* curl = curl_easy_init();

    if (!curl)
    {
        std::cout << "Failed to initialize CURL" << std::endl;
        return false;
    }

    std::string json =
        "{"
        "\"orderId\":" + std::to_string(orderId) + ","
        "\"amount\":" + std::to_string(amount) + ","
        "\"userId\":" + std::to_string(userId) + ","
        "\"paymentMethod\":\"CARD\""
        "}";

    std::cout << "\n===============================" << std::endl;
    std::cout << "Sending JSON to PaymentService" << std::endl;
    std::cout << json << std::endl;
    std::cout << "===============================\n" << std::endl;

    struct curl_slist* headers = nullptr;

    headers = curl_slist_append(
        headers,
        "Content-Type: application/json");
    const std::string idempotencyHeader = "Idempotency-Key: order-" + std::to_string(orderId);
    headers = curl_slist_append(headers, idempotencyHeader.c_str());

    const char* configuredUrl = std::getenv("PAYMENT_SERVICE_URL");
    const std::string url = std::string(configuredUrl ? configuredUrl : "http://localhost:8083") + "/payments";
    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        url.c_str());

    curl_easy_setopt(
        curl,
        CURLOPT_POST,
        1L);

    curl_easy_setopt(
        curl,
        CURLOPT_HTTPHEADER,
        headers);

    curl_easy_setopt(
        curl,
        CURLOPT_POSTFIELDS,
        json.c_str());

    CURLcode res = curl_easy_perform(curl);

    long responseCode = 0;

    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &responseCode);

    std::cout
        << "HTTP Response Code : "
        << responseCode
        << std::endl;

    if (res != CURLE_OK)
    {
        std::cout
            << "Curl Error : "
            << curl_easy_strerror(res)
            << std::endl;
    }

    curl_slist_free_all(headers);

    curl_easy_cleanup(curl);

    return (res == CURLE_OK && responseCode == 201);
}

bool PaymentClient::isPaymentSucceeded(int orderId)
{
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    std::string response;
    const char* configuredUrl = std::getenv("PAYMENT_SERVICE_URL");
    const std::string url = std::string(configuredUrl ? configuredUrl : "http://localhost:8083") +
        "/payments/order/" + std::to_string(orderId);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, paymentResponseCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    const CURLcode result = curl_easy_perform(curl);
    long status = 0; curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_cleanup(curl);
    if (result != CURLE_OK || status != 200) {
        std::clog << "[delivery-gate] payment lookup failed order=" << orderId
                  << " http=" << status << " curl=" << static_cast<int>(result) << std::endl;
        return false;
    }
    const auto json = crow::json::load(response);
    const std::string paymentStatus = json && json.has("status") ? std::string(json["status"].s()) : "invalid-response";
    const bool paid = paymentStatus == "succeeded";
    std::clog << "[delivery-gate] order=" << orderId << " paymentStatus=" << paymentStatus
              << " decision=" << (paid ? "allow" : "deny") << std::endl;
    return paid;
}
