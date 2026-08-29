#include "client/HttpClient.h"

#include <cassert>
#include <iostream>

int main()
{
    assert(gatewayStatusFor({200, "{}", TransportFailure::None, ""}) == 200);
    assert(gatewayStatusFor({401, "{}", TransportFailure::None, ""}) == 401);
    assert(gatewayStatusFor({503, "{}", TransportFailure::None, ""}) == 503);
    assert(gatewayStatusFor({0, "", TransportFailure::Timeout, "timeout"}) == 504);
    assert(gatewayStatusFor({0, "", TransportFailure::Connection, "connect"}) == 502);
    assert(gatewayStatusFor({0, "", TransportFailure::Other, "other"}) == 502);
    assert(gatewayStatusFor({0, "", TransportFailure::None, ""}) == 502);
    std::cout << "HTTP result mapping tests passed\n";
}
