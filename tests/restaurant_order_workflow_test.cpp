#include "RestaurantOrderWorkflow.h"

#include <cassert>
#include <string>

int main()
{
    using namespace restaurant_order;
    assert(isPaidOrderStatus("CONFIRMED"));
    assert(isPaidOrderStatus("DELIVERED"));
    assert(!isPaidOrderStatus("PAYMENT_PENDING"));
    assert(initialStatus("CONFIRMED") == Status::New);
    assert(initialStatus("ASSIGNED") == Status::ReadyForPickup);
    assert(initialStatus("PICKED_UP") == Status::HandedOff);
    assert(canTransition(Status::New, Status::Accepted, false).allowed);
    assert(canTransition(Status::Accepted, Status::Preparing, false).allowed);
    assert(canTransition(Status::Preparing, Status::ReadyForPickup, false).allowed);
    assert(!canTransition(Status::ReadyForPickup, Status::HandedOff, false).allowed);
    assert(canTransition(Status::ReadyForPickup, Status::HandedOff, true).allowed);
    assert(!canTransition(Status::New, Status::Preparing, false).allowed);
    assert(!nextStatus(Status::HandedOff));
}
