#pragma once
#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
namespace partner {
enum class Role{Owner,Manager,Staff,Unknown};
enum class Status{Draft,PendingReview,Approved,Suspended,Rejected};
inline Role parseRole(std::string_view value){
 if(value=="OWNER")return Role::Owner;
 if(value=="MANAGER")return Role::Manager;
 if(value=="STAFF")return Role::Staff;
 return Role::Unknown;
}
enum class Permission{View,EditRestaurant,EditMenu,ManageOrders,ManageStaff,Submit};
inline bool allowed(Role r,Permission p){
 if(r==Role::Owner)return true;
 if(r==Role::Manager)return p==Permission::View||p==Permission::EditRestaurant||p==Permission::EditMenu||p==Permission::ManageOrders||p==Permission::Submit;
 return r==Role::Staff&&(p==Permission::View||p==Permission::EditMenu||p==Permission::ManageOrders);
}
inline bool canTransition(Status from,Status to){
 return (from==Status::Draft&&to==Status::PendingReview)||(from==Status::Rejected&&to==Status::Draft);
}
inline bool validName(std::string_view v){return v.size()>=2&&v.size()<=120&&std::any_of(v.begin(),v.end(),[](unsigned char c){return std::isalnum(c);});}
inline bool validPricePaise(long long v){return v>=0&&v<=100000000;}
inline bool validCoordinates(double lat,double lon){return lat>=-90&&lat<=90&&lon>=-180&&lon<=180;}
}
