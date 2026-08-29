#pragma once

#include "CorrelationId.h"
#include <crow.h>

struct CorrelationMiddleware
{
    struct context { std::string id; };

    void before_handle(crow::request& request, crow::response&, context& context)
    {
        context.id = chooseCorrelationId(request.get_header_value("X-Correlation-ID"));
        currentCorrelationId = context.id;
        request.headers.erase("X-Correlation-ID");
        request.add_header("X-Correlation-ID", context.id);
    }

    void after_handle(crow::request& request, crow::response& response, context& context)
    {
        response.set_header("X-Correlation-ID", context.id);
        currentCorrelationId.clear();
    }
};
