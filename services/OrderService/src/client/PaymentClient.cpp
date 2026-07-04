#include "client/PaymentClient.h"
#include <iostream>
#include <curl/curl.h>
#include <iostream>

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

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        "http://localhost:8083/payments");

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
