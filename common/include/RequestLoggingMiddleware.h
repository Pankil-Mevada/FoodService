#pragma once

#include <crow.h>

#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>

inline crow::LogLevel configuredLogLevel()
{
    const char* configured=std::getenv("FOODSERVICE_LOG_LEVEL");
    const std::string level=configured ? configured : "INFO";
    if (level=="DEBUG") return crow::LogLevel::Debug;
    if (level=="WARNING" || level=="WARN") return crow::LogLevel::Warning;
    if (level=="ERROR") return crow::LogLevel::Error;
    if (level=="CRITICAL") return crow::LogLevel::Critical;
    return crow::LogLevel::Info;
}

struct RequestLoggingMiddleware
{
    struct context
    {
        std::chrono::steady_clock::time_point started;
        std::string correlationId;
    };

    static void setServiceName(std::string name) { serviceName()=std::move(name); }

    void before_handle(crow::request& request, crow::response&, context& value)
    {
        value.started=std::chrono::steady_clock::now();
        value.correlationId=safeCorrelationId(request.get_header_value("X-Correlation-ID"));
        const char* configured=std::getenv("FOODSERVICE_LOG_LEVEL");
        if (configured && std::string(configured)=="DEBUG")
            write("DEBUG","event=request.received service="+serviceName()+
                " correlationId="+value.correlationId+" method="+
                crow::method_name(request.method)+" path="+request.url);
    }

    void after_handle(crow::request& request, crow::response& response, context& value)
    {
        const auto duration=std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now()-value.started).count();
        const std::string message="event=request.completed service="+serviceName()+
            " correlationId="+value.correlationId+" method="+crow::method_name(request.method)+
            " path="+request.url+" status="+std::to_string(response.code)+
            " durationMs="+std::to_string(duration);
        if (response.code>=500) write("ERROR",message);
        else if (response.code>=400) write("WARNING",message);
        else write("INFO",message);
    }

private:
    static std::string& serviceName() { static std::string name="unknown"; return name; }
    static void write(const char* level,const std::string& message)
    {
        const auto rank=[](const std::string& value)
        {
            if (value=="DEBUG") return 0;
            if (value=="INFO") return 1;
            if (value=="WARNING" || value=="WARN") return 2;
            if (value=="ERROR") return 3;
            return 4;
        };
        const char* configured=std::getenv("FOODSERVICE_LOG_LEVEL");
        if (rank(level)<rank(configured ? configured : "INFO")) return;
        static std::mutex outputMutex;
        std::lock_guard<std::mutex> lock(outputMutex);
        std::clog << '[' << level << "] " << message << std::endl;
    }
    static std::string safeCorrelationId(const std::string& supplied)
    {
        if (!supplied.empty() && supplied.size()<=64)
        {
            bool safe=true;
            for (const unsigned char value:supplied)
                safe=safe && (std::isalnum(value) || value=='-' || value=='_' || value=='.');
            if (safe) return supplied;
        }
        static std::atomic<unsigned long long> sequence{0};
        return "local-"+std::to_string(sequence.fetch_add(1,std::memory_order_relaxed));
    }
};
