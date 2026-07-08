#pragma once

#include <crow.h>

#include "JwtManager.h"

struct JwtMiddleware
{
    struct context
    {
    };

    void before_handle(
        crow::request& req,
        crow::response& res,
        context&)
    {
        // Public APIs
        if (req.url == "/login" ||
            req.url == "/register" ||
            req.url == "/health")
        {
            return;
        }

        auto authHeader = req.get_header_value("Authorization");
        std::cout << "UserService received header: ["
          << authHeader << "]" << std::endl;

        if (authHeader.empty())
        {
            res.code = 401;
            res.write("Missing Authorization Header");
            res.end();
            return;
        }

        const std::string prefix = "Bearer ";

        if (authHeader.substr(0, prefix.size()) != prefix)
        {
            res.code = 401;
            res.write("Invalid Authorization Header");
            res.end();
            return;
        }

        std::string token =
            authHeader.substr(prefix.size());

        JwtManager jwt;

        if (!jwt.verifyToken(token))
        {
            res.code = 401;
            res.write("Invalid Token");
            res.end();
            return;
        }
    }

    void after_handle(
        crow::request&,
        crow::response&,
        context&)
    {
    }
};
