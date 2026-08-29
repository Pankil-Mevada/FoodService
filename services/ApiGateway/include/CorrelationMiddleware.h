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
        CROW_LOG_WARNING << "request.start correlationId=" << context.id
                         << " method=" << crow::method_name(request.method)
                         << " url=" << request.url;
    }

    void after_handle(crow::request& request, crow::response& response, context& context)
    {
        response.set_header("X-Correlation-ID", context.id);
        CROW_LOG_WARNING << "request.finish correlationId=" << context.id
                         << " method=" << crow::method_name(request.method)
                         << " url=" << request.url << " status=" << response.code;
        currentCorrelationId.clear();
    }
};
