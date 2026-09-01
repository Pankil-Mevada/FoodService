#include "PartnerAccessPolicy.h"
#include <cassert>
int main(){using namespace partner;
 assert(allowed(Role::Owner,Permission::ManageStaff));
 assert(allowed(Role::Manager,Permission::EditMenu));
 assert(!allowed(Role::Manager,Permission::ManageStaff));
 assert(!allowed(Role::Staff,Permission::EditRestaurant));
 assert(canTransition(Status::Draft,Status::PendingReview));
 assert(!canTransition(Status::Draft,Status::Approved));
 assert(!canTransition(Status::Approved,Status::Draft));
 assert(validName("Spice Route"));assert(!validName(" "));
 assert(validPricePaise(0));assert(!validPricePaise(-1));
 assert(validCoordinates(12.9716,77.5946));assert(!validCoordinates(91,77));
}
