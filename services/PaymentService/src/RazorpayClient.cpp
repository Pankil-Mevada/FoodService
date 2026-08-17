#include "RazorpayClient.h"
#include <crow.h>
#include <curl/curl.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
size_t writeBody(void* contents, size_t size, size_t count, void* target) {
    static_cast<std::string*>(target)->append(static_cast<char*>(contents), size * count); return size * count;
}
std::string env(const char* name) { const char* value = std::getenv(name); return value ? value : ""; }
bool safeEqual(const std::string& a, const std::string& b) {
    size_t difference = a.size() ^ b.size();
    for (size_t i = 0; i < std::max(a.size(), b.size()); ++i)
        difference |= static_cast<unsigned char>(i < a.size() ? a[i] : 0) ^ static_cast<unsigned char>(i < b.size() ? b[i] : 0);
    return difference == 0;
}
}

bool RazorpayClient::configured() const { return !env("RAZORPAY_KEY_ID").empty() && !env("RAZORPAY_KEY_SECRET").empty(); }
std::string RazorpayClient::keyId() const { return env("RAZORPAY_KEY_ID"); }

std::optional<RazorpayOrder> RazorpayClient::createOrder(long long amountPaise, const std::string& receipt,
                                                         std::string& error) const {
    std::clog << "[razorpay] create-order start receipt=" << receipt << " amountPaise=" << amountPaise << std::endl;
    if (!configured()) { error = "Razorpay Test Mode is not configured"; std::clog << "[razorpay] create-order rejected: credentials missing" << std::endl; return std::nullopt; }
    CURL* curl = curl_easy_init();
    if (!curl) { error = "Could not initialize payment provider"; return std::nullopt; }
    crow::json::wvalue request; request["amount"] = amountPaise; request["currency"] = "INR"; request["receipt"] = receipt;
    const std::string body = request.dump(); std::string response;
    struct curl_slist* headers = curl_slist_append(nullptr, "Content-Type: application/json");
    const std::string credentials = keyId() + ":" + env("RAZORPAY_KEY_SECRET");
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.razorpay.com/v1/orders");
    curl_easy_setopt(curl, CURLOPT_USERPWD, credentials.c_str()); curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeBody); curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    const CURLcode result = curl_easy_perform(curl); long status = 0; curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_slist_free_all(headers); curl_easy_cleanup(curl);
    if (result != CURLE_OK || status < 200 || status >= 300) { error = "Razorpay order creation failed"; std::clog << "[razorpay] create-order provider failure http=" << status << " curl=" << static_cast<int>(result) << std::endl; return std::nullopt; }
    const auto json = crow::json::load(response);
    if (!json || !json.has("id") || !json.has("amount") || !json.has("currency")) { error = "Razorpay returned an invalid order"; return std::nullopt; }
    std::clog << "[razorpay] create-order accepted providerOrderId=" << json["id"].s() << std::endl;
    return RazorpayOrder{json["id"].s(), json["amount"].i(), json["currency"].s()};
}

bool RazorpayClient::verifyPaymentSignature(const std::string& orderId, const std::string& paymentId,
                                            const std::string& signature) const {
    if (!configured() || orderId.empty() || paymentId.empty() || signature.empty()) return false;
    const std::string data = orderId + "|" + paymentId, secret = env("RAZORPAY_KEY_SECRET");
    unsigned char digest[EVP_MAX_MD_SIZE]; unsigned int length = 0;
    HMAC(EVP_sha256(), secret.data(), static_cast<int>(secret.size()), reinterpret_cast<const unsigned char*>(data.data()), data.size(), digest, &length);
    std::ostringstream expected; expected << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < length; ++i) expected << std::setw(2) << static_cast<int>(digest[i]);
    const bool verified = safeEqual(expected.str(), signature);
    std::clog << "[razorpay] signature-verification " << (verified ? "passed" : "failed")
              << " orderId=" << orderId << std::endl;
    return verified;
}
