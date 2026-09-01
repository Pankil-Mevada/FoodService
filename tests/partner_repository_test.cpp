#include "Database.h"
#include "PartnerRepository.h"
#include "RestaurantRepository.h"

#include <cassert>
#include <filesystem>
#include <string>
#include <unistd.h>

int main() {
    const auto path=(std::filesystem::temp_directory_path()/
        ("foodservice-partner-"+std::to_string(getpid())+".db")).string();
    {
        Database database(path);
        database.createRestaurantTable();
        PartnerRepository partners(database);
        assert(partners.ready());

        PartnerRestaurantRecord restaurant;
        restaurant.name="Secure Kitchen"; restaurant.address="1 Test Road";
        restaurant.phone="9999999999"; restaurant.latitude=12.9716;
        restaurant.longitude=77.5946;
        int restaurantId=0;
        assert(partners.createRestaurant(101,restaurant,"test-create",restaurantId)
            ==PartnerWriteResult::Ok);
        assert(restaurantId>0);
        assert(partners.listRestaurants(101).size()==1);
        assert(partners.listRestaurants(202).empty());

        restaurant.id=restaurantId; restaurant.name="Secure Kitchen Updated";
        assert(partners.updateRestaurant(202,restaurant,1,"cross-tenant")
            ==PartnerWriteResult::NotFound);
        assert(partners.updateRestaurant(101,restaurant,1,"owner-update")
            ==PartnerWriteResult::Ok);
        assert(restaurant.version==2);
        assert(partners.updateRestaurant(101,restaurant,1,"stale-update")
            ==PartnerWriteResult::Conflict);

        PartnerMenuItemRecord item; item.restaurantId=restaurantId;
        item.name="Masala Dosa"; item.description="Fresh test item";
        item.pricePaise=17900; item.dietType="VEG";
        assert(partners.createMenuItem(202,item,"foreign-item")
            ==PartnerWriteResult::NotFound);
        assert(partners.createMenuItem(101,item,"owner-item")
            ==PartnerWriteResult::Ok);
        assert(item.id>0);

        int submittedVersion=0;
        assert(partners.submitRestaurant(101,restaurantId,2,"submit",submittedVersion)
            ==PartnerWriteResult::Ok);
        assert(submittedVersion==3);
        assert(partners.updateRestaurant(101,restaurant,3,"pending-edit")
            ==PartnerWriteResult::InvalidState);

        RestaurantRepository customerRepository(database);
        assert(customerRepository.getAllRestaurants().empty());
        assert(!customerRepository.getRestaurantById(restaurantId));

        assert(database.execute("UPDATE restaurants SET onboarding_status='APPROVED' "
            "WHERE id="+std::to_string(restaurantId)+";"));
        assert(customerRepository.getAllRestaurants().size()==1);
        assert(customerRepository.getRestaurantById(restaurantId).has_value());
        assert(partners.listAudit(101,restaurantId,20).size()>=4);
        assert(partners.listAudit(202,restaurantId,20).empty());
    }
    std::filesystem::remove(path);
    std::filesystem::remove(path+"-wal");
    std::filesystem::remove(path+"-shm");
}
