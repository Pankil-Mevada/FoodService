#include "CorrelationId.h"

#include <cassert>

int main()
{
    assert(validCorrelationId("checkout-123"));
    assert(validCorrelationId("mobile.app_request-9"));
    assert(!validCorrelationId(""));
    assert(!validCorrelationId("contains spaces"));
    assert(!validCorrelationId("contains\r\nheader"));
    assert(!validCorrelationId(std::string(65, 'a')));
    assert(chooseCorrelationId("client-42") == "client-42");
    const auto first = chooseCorrelationId("bad value");
    const auto second = chooseCorrelationId("");
    assert(validCorrelationId(first));
    assert(validCorrelationId(second));
    assert(first != second);
}
