#include "client/OrderClient.h"

#include <curl/curl.h>

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
std::string environment(const char* name, const char* fallback)
{
    const char* value = std::getenv(name);
    return value && *value ? value : fallback;
}
size_t discardBody(void*, size_t size, size_t count, void*)
{
    return size * count;
}
}

bool OrderClient::synchronizePaymentStatus(
    int orderId,
    const std::string& paymentStatus) const
{
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    const std::string url = environment("ORDER_SERVICE_URL", "http://localhost:8082") +
        "/orders/" + std::to_string(orderId) + "/payment-status";
    const std::string body = "{\"paymentStatus\":\"" + paymentStatus + "\"}";
    const std::string secretHeader = "X-Internal-Secret: " +
        environment("ORDER_SYNC_SECRET", "local-order-sync-secret");

    curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, secretHeader.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discardBody);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 1000L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 3000L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    const CURLcode result = curl_easy_perform(curl);
    long responseCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    const bool synchronized = result == CURLE_OK && responseCode == 200;
    std::clog << "[payment-order-sync] order=" << orderId
              << " paymentStatus=" << paymentStatus
              << " http=" << responseCode
              << " result=" << (synchronized ? "synchronized" : "failed")
              << std::endl;
    return synchronized;
}
