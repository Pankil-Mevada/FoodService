#include "client/NotificationClient.h"

#include <curl/curl.h>

#include <iostream>
#include <string>

bool NotificationClient::createNotification(
    int userId,
    const std::string& message)
{
    CURL* curl = curl_easy_init();

    if (!curl)
    {
        return false;
    }

    std::string json =
        "{"
        "\"userId\":" + std::to_string(userId) + ","
        "\"type\":\"PAYMENT\","
        "\"message\":\"" + message + "\""
        "}";

    std::cout << "\nSending Notification Request\n";
    std::cout << json << std::endl;

    struct curl_slist* headers = nullptr;

    headers = curl_slist_append(
        headers,
        "Content-Type: application/json");

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        "http://localhost:8084/notifications");

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
        << "Notification Response : "
        << responseCode
        << std::endl;

    curl_slist_free_all(headers);

    curl_easy_cleanup(curl);

    return (res == CURLE_OK &&
            responseCode == 201);
}
