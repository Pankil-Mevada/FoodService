#pragma once
#include <cmath>
#include <string>
#include <vector>

struct DeliveryPoint { double latitude{}, longitude{}; };
struct DeliveryRules {
    double radiusKm{8.0}, baseFee{39.0}, perKmFee{5.0};
    int preparationMinutes{20};
    bool surge{}, rain{}, lateNight{};
};
struct DeliveryQuote {
    bool serviceable{}; double distanceKm{}, fee{}; int etaMinutes{};
    std::string zoneType{"radius"};
};

inline double deliveryDistanceKm(DeliveryPoint a, DeliveryPoint b)
{
    constexpr double pi=3.14159265358979323846, earth=6371.0;
    auto rad=[](double v){ return v*pi/180.0; };
    const double dlat=rad(b.latitude-a.latitude), dlon=rad(b.longitude-a.longitude);
    const double value=std::sin(dlat/2)*std::sin(dlat/2)+std::cos(rad(a.latitude))*std::cos(rad(b.latitude))*std::sin(dlon/2)*std::sin(dlon/2);
    return earth*2*std::atan2(std::sqrt(value),std::sqrt(1-value));
}

inline bool pointInPolygon(DeliveryPoint point, const std::vector<DeliveryPoint>& polygon)
{
    if (polygon.size()<3) return false;
    bool inside=false;
    for (std::size_t i=0,j=polygon.size()-1;i<polygon.size();j=i++) {
        const auto& a=polygon[i]; const auto& b=polygon[j];
        const bool crosses=((a.longitude>point.longitude)!=(b.longitude>point.longitude)) &&
            point.latitude<(b.latitude-a.latitude)*(point.longitude-a.longitude)/(b.longitude-a.longitude)+a.latitude;
        if(crosses) inside=!inside;
    }
    return inside;
}

inline DeliveryQuote calculateDeliveryQuote(DeliveryPoint restaurant, DeliveryPoint customer,
    const DeliveryRules& rules, const std::vector<DeliveryPoint>& polygon={})
{
    DeliveryQuote quote; quote.distanceKm=deliveryDistanceKm(restaurant,customer);
    quote.zoneType=polygon.size()>=3?"polygon":"radius";
    quote.serviceable=polygon.size()>=3?pointInPolygon(customer,polygon):quote.distanceKm<=rules.radiusKm;
    double multiplier=(rules.surge?1.25:1.0)*(rules.rain?1.15:1.0)*(rules.lateNight?1.20:1.0);
    quote.fee=std::round((rules.baseFee+quote.distanceKm*rules.perKmFee)*multiplier*100.0)/100.0;
    quote.etaMinutes=rules.preparationMinutes+static_cast<int>(std::ceil(quote.distanceKm/20.0*60.0))+5+(rules.rain?8:0)+(rules.surge?5:0);
    return quote;
}
