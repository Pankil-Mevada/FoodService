#include "DeliveryQuote.h"
#include <cassert>
#include <iostream>
int main() {
    DeliveryRules rules; rules.radiusKm=8; rules.rain=true;
    const auto near=calculateDeliveryQuote({12.9716,77.5946},{12.98,77.60},rules);
    assert(near.serviceable && near.distanceKm<8 && near.fee>39 && near.etaMinutes>25);
    const auto far=calculateDeliveryQuote({12.9716,77.5946},{13.20,77.90},rules);
    assert(!far.serviceable);
    std::vector<DeliveryPoint> polygon={{12.9,77.5},{12.9,77.7},{13.1,77.7},{13.1,77.5}};
    assert(calculateDeliveryQuote({12.97,77.59},{12.98,77.60},rules,polygon).serviceable);
    assert(!calculateDeliveryQuote({12.97,77.59},{13.2,77.8},rules,polygon).serviceable);
    std::cout << "delivery quote tests passed\n";
}
