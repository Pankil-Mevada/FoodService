#include "client/HttpClient.h"
#include<iostream>
#include <curl/curl.h>

namespace
{
size_t WriteCallback(
    void* contents,
    size_t size,
    size_t nmemb,
    void* userp)
{
    ((std::string*)userp)->append(
        (char*)contents,
        size * nmemb);

    return size * nmemb;
}
}

std::string HttpClient::get(
    const std::string& url,
    const std::string& authHeader)
{
    CURL* curl = curl_easy_init();

    std::string response;

    if (curl)
    {
        struct curl_slist* headers = nullptr;

        std::cout << "Sending header: "
          << authHeader << std::endl;

        if (!authHeader.empty())
        {
           std::string header =
            "Authorization: " + authHeader;

             headers = curl_slist_append(
                         headers,
                         header.c_str());

            curl_easy_setopt(
                curl,
                CURLOPT_HTTPHEADER,
                headers);
        }

        curl_easy_setopt(
            curl,
            CURLOPT_URL,
            url.c_str());

        curl_easy_setopt(
            curl,
            CURLOPT_WRITEFUNCTION,
            WriteCallback);

        curl_easy_setopt(
            curl,
            CURLOPT_WRITEDATA,
            &response);

        curl_easy_perform(curl);

        if (headers)
        {
            curl_slist_free_all(headers);
        }

        curl_easy_cleanup(curl);
    }

    return response;
}

std::string HttpClient::put(
    const std::string& url,
    const std::string& body,
    const std::string& authHeader)
{
    CURL* curl = curl_easy_init();

    std::string response;

    if (curl)
    {
        struct curl_slist* headers = nullptr;

        headers = curl_slist_append(
            headers,
            "Content-Type: application/json");

        if (!authHeader.empty())
            {
                std::string header =
                    "Authorization: " + authHeader;

                headers = curl_slist_append(
                    headers,
                    header.c_str());
            }

        curl_easy_setopt(
            curl,
            CURLOPT_URL,
            url.c_str());

        curl_easy_setopt(
            curl,
            CURLOPT_CUSTOMREQUEST,
            "PUT");

        curl_easy_setopt(
            curl,
            CURLOPT_HTTPHEADER,
            headers);

        curl_easy_setopt(
            curl,
            CURLOPT_POSTFIELDS,
            body.c_str());

        curl_easy_setopt(
            curl,
            CURLOPT_WRITEFUNCTION,
            WriteCallback);

        curl_easy_setopt(
            curl,
            CURLOPT_WRITEDATA,
            &response);

        curl_easy_perform(curl);

        curl_slist_free_all(headers);

        curl_easy_cleanup(curl);
    }

    return response;
}

std::string HttpClient::remove(
    const std::string& url,
    const std::string& authHeader)
{
    CURL* curl = curl_easy_init();

    std::string response;

    if (curl)
    {
        struct curl_slist* headers = nullptr;

        if (!authHeader.empty())
        {
            std::string header =
                "Authorization: " + authHeader;

            headers = curl_slist_append(
                headers,
                header.c_str());

            curl_easy_setopt(
                curl,
                CURLOPT_HTTPHEADER,
                headers);
        }

        curl_easy_setopt(
            curl,
            CURLOPT_URL,
            url.c_str());

        curl_easy_setopt(
            curl,
            CURLOPT_CUSTOMREQUEST,
            "DELETE");

        curl_easy_setopt(
            curl,
            CURLOPT_WRITEFUNCTION,
            WriteCallback);

        curl_easy_setopt(
            curl,
            CURLOPT_WRITEDATA,
            &response);

        curl_easy_perform(curl);

        if (headers)
        {
            curl_slist_free_all(headers);
        }

        curl_easy_cleanup(curl);
    }

    return response;
}

std::string HttpClient::post(
    const std::string& url,
    const std::string& body,
    const std::string& authHeader)
{
    CURL* curl = curl_easy_init();

    std::string response;

    if (curl)
    {
        struct curl_slist* headers = nullptr;

        headers = curl_slist_append(
            headers,
            "Content-Type: application/json");

        if (!authHeader.empty())
        {
            std::string header =
                "Authorization: " + authHeader;

            headers = curl_slist_append(
                headers,
                header.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        curl_easy_perform(curl);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return response;
}