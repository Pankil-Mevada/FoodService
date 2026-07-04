#include "client/RestaurantClient.h"
#include <curl/curl.h>
#include <iostream>

bool RestaurantClient::restaurantExists(int restaurantId)
{
    CURL* curl = curl_easy_init();

    if (!curl)
    {
        return false;
    }

    std::string url =
        "http://localhost:8081/restaurants/" +
        std::to_string(restaurantId);

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        url.c_str());

    CURLcode result =
        curl_easy_perform(curl);

    long responseCode = 0;

    curl_easy_getinfo(
        curl,
        CURLINFO_RESPONSE_CODE,
        &responseCode);

    curl_easy_cleanup(curl);

    std::cout
        << "RestaurantService Response : "
        << responseCode
        << std::endl;

    return (result == CURLE_OK &&
            responseCode == 200);
}
