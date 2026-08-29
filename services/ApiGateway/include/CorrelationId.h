#pragma once

#include <atomic>
#include <chrono>
#include <cctype>
#include <sstream>
#include <string>
#include <string_view>

inline thread_local std::string currentCorrelationId;

inline bool validCorrelationId(std::string_view value)
{
    if (value.empty() || value.size() > 64) return false;
    for (const unsigned char character : value)
        if (!(std::isalnum(character) || character == '-' || character == '_' || character == '.'))
            return false;
    return true;
}

inline std::string generateCorrelationId()
{
    static std::atomic<unsigned long long> sequence{0};
    const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::ostringstream value;
    value << "fs-" << std::hex << micros << '-' << sequence.fetch_add(1, std::memory_order_relaxed);
    return value.str();
}

inline std::string chooseCorrelationId(std::string_view supplied)
{
    return validCorrelationId(supplied) ? std::string(supplied) : generateCorrelationId();
}
