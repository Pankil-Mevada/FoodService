#include "PartnerController.h"
#include "JwtManager.h"
#include "PartnerAccessPolicy.h"

#include <algorithm>
#include <optional>

namespace {
crow::response error(int status, const std::string& message) {
    crow::json::wvalue body; body["success"]=false; body["message"]=message;
    return crow::response(status,body);
}
std::optional<int> actorId(const crow::request& request) {
    const std::string header=request.get_header_value("Authorization");
    if(header.rfind("Bearer ",0)!=0) return std::nullopt;
    const std::string token=header.substr(7); JwtManager jwt;
    if(!jwt.verifyToken(token)) return std::nullopt;
    try{return jwt.getUserId(token);}catch(...){return std::nullopt;}
}
std::string correlationId(const crow::request& request) {
    return request.get_header_value("X-Correlation-ID");
}
crow::response writeResponse(PartnerWriteResult result,const std::string& success,
    int successStatus=200) {
    if(result==PartnerWriteResult::Ok){
        crow::json::wvalue body; body["success"]=true; body["message"]=success;
        return crow::response(successStatus,body);
    }
    if(result==PartnerWriteResult::NotFound) return error(404,"Restaurant or resource not found");
    if(result==PartnerWriteResult::Forbidden) return error(403,"Your partner role cannot perform this action");
    if(result==PartnerWriteResult::Conflict) return error(409,"This record changed; reload before saving again");
    if(result==PartnerWriteResult::InvalidState) return error(409,"This action is not allowed in the current restaurant state");
    if(result==PartnerWriteResult::Invalid) return error(422,"The restaurant is not ready for this action");
    return error(500,"The partner operation could not be completed");
}
crow::json::wvalue restaurantJson(const PartnerRestaurantRecord& value) {
    crow::json::wvalue row; row["id"]=value.id; row["name"]=value.name;
    row["address"]=value.address; row["phone"]=value.phone;
    row["latitude"]=value.latitude; row["longitude"]=value.longitude;
    row["deliveryRadiusKm"]=value.deliveryRadiusKm; row["imageUrl"]=value.imageUrl;
    row["baseDeliveryFee"]=value.baseDeliveryFee; row["perKmFee"]=value.perKmFee;
    row["preparationMinutes"]=value.preparationMinutes; row["status"]=value.status;
    row["role"]=value.role; row["version"]=value.version; return row;
}
crow::json::wvalue menuJson(const PartnerMenuItemRecord& value) {
    crow::json::wvalue row; row["id"]=value.id; row["restaurantId"]=value.restaurantId;
    row["name"]=value.name; row["description"]=value.description;
    row["pricePaise"]=value.pricePaise; row["dietType"]=value.dietType;
    row["available"]=value.available; row["version"]=value.version; return row;
}
bool validRestaurant(const PartnerRestaurantRecord& value) {
    return partner::validName(value.name)&&!value.address.empty()&&value.address.size()<=300&&
        value.phone.size()>=7&&value.phone.size()<=24&&
        partner::validCoordinates(value.latitude,value.longitude)&&
        value.deliveryRadiusKm>=0.5&&value.deliveryRadiusKm<=50&&
        value.baseDeliveryFee>=0&&value.baseDeliveryFee<=10000&&
        value.perKmFee>=0&&value.perKmFee<=1000&&
        value.preparationMinutes>=1&&value.preparationMinutes<=240&&
        value.imageUrl.size()<=1000;
}
bool validDiet(const std::string& value) {
    return value=="VEG"||value=="NON_VEG"||value=="VEGAN";
}
}

PartnerController::PartnerController(PartnerRepository& repository):m_repository(repository){}

crow::response PartnerController::listRestaurants(const crow::request& request) {
    const auto actor=actorId(request); if(!actor) return error(401,"A valid bearer token is required");
    const auto rows=m_repository.listRestaurants(*actor);
    crow::json::wvalue body(crow::json::wvalue::list{});
    std::size_t index=0; for(const auto& row:rows) body[index++]=restaurantJson(row);
    return crow::response(body);
}

crow::response PartnerController::createRestaurant(const crow::request& request) {
    const auto actor=actorId(request); if(!actor) return error(401,"A valid bearer token is required");
    const auto json=crow::json::load(request.body);
    if(!json||!json.has("name")||!json.has("address")||!json.has("phone")||
        !json.has("latitude")||!json.has("longitude"))
        return error(400,"Name, address, phone and coordinates are required");
    PartnerRestaurantRecord value; value.name=json["name"].s(); value.address=json["address"].s();
    value.phone=json["phone"].s(); value.latitude=json["latitude"].d();
    value.longitude=json["longitude"].d();
    if(json.has("deliveryRadiusKm")) value.deliveryRadiusKm=json["deliveryRadiusKm"].d();
    if(json.has("imageUrl")) value.imageUrl=json["imageUrl"].s();
    if(json.has("baseDeliveryFee")) value.baseDeliveryFee=json["baseDeliveryFee"].d();
    if(json.has("perKmFee")) value.perKmFee=json["perKmFee"].d();
    if(json.has("preparationMinutes")) value.preparationMinutes=json["preparationMinutes"].i();
    if(!validRestaurant(value)) return error(422,"Restaurant details are invalid");
    int id=0; const auto result=m_repository.createRestaurant(*actor,value,correlationId(request),id);
    if(result!=PartnerWriteResult::Ok) return writeResponse(result,"",201);
    crow::json::wvalue body; body["success"]=true; body["restaurantId"]=id;
    body["status"]="DRAFT"; body["version"]=1; return crow::response(201,body);
}

crow::response PartnerController::getRestaurant(const crow::request& request,int restaurantId) {
    const auto actor=actorId(request); if(!actor) return error(401,"A valid bearer token is required");
    const auto row=m_repository.getRestaurant(*actor,restaurantId);
    if(!row) return error(404,"Restaurant not found"); return crow::response(restaurantJson(*row));
}

crow::response PartnerController::updateRestaurant(const crow::request& request,int restaurantId) {
    const auto actor=actorId(request); if(!actor) return error(401,"A valid bearer token is required");
    const auto json=crow::json::load(request.body);
    if(!json||!json.has("version")||!json.has("name")||!json.has("address")||
        !json.has("phone")||!json.has("latitude")||!json.has("longitude"))
        return error(400,"Version and complete restaurant details are required");
    PartnerRestaurantRecord value; value.id=restaurantId; value.name=json["name"].s();
    value.address=json["address"].s(); value.phone=json["phone"].s();
    value.latitude=json["latitude"].d(); value.longitude=json["longitude"].d();
    if(json.has("deliveryRadiusKm")) value.deliveryRadiusKm=json["deliveryRadiusKm"].d();
    if(json.has("imageUrl")) value.imageUrl=json["imageUrl"].s();
    if(json.has("baseDeliveryFee")) value.baseDeliveryFee=json["baseDeliveryFee"].d();
    if(json.has("perKmFee")) value.perKmFee=json["perKmFee"].d();
    if(json.has("preparationMinutes")) value.preparationMinutes=json["preparationMinutes"].i();
    if(!validRestaurant(value)) return error(422,"Restaurant details are invalid");
    const auto result=m_repository.updateRestaurant(*actor,value,json["version"].i(),correlationId(request));
    if(result!=PartnerWriteResult::Ok) return writeResponse(result,"");
    const auto updated=m_repository.getRestaurant(*actor,restaurantId);
    if(!updated) return error(500,"Restaurant was updated but could not be reloaded");
    auto body=restaurantJson(*updated); body["success"]=true; return crow::response(body);
}

crow::response PartnerController::submitRestaurant(const crow::request& request,int restaurantId) {
    const auto actor=actorId(request); if(!actor) return error(401,"A valid bearer token is required");
    const auto json=crow::json::load(request.body);
    if(!json||!json.has("version")) return error(400,"Version is required");
    int version=0; const auto result=m_repository.submitRestaurant(*actor,restaurantId,
        json["version"].i(),correlationId(request),version);
    if(result!=PartnerWriteResult::Ok) return writeResponse(result,"");
    crow::json::wvalue body; body["success"]=true; body["status"]="PENDING_REVIEW";
    body["version"]=version; return crow::response(body);
}

crow::response PartnerController::listMenuItems(const crow::request& request,int restaurantId) {
    const auto actor=actorId(request); if(!actor) return error(401,"A valid bearer token is required");
    if(!m_repository.getRestaurant(*actor,restaurantId)) return error(404,"Restaurant not found");
    const auto rows=m_repository.listMenuItems(*actor,restaurantId);
    crow::json::wvalue body(crow::json::wvalue::list{});
    std::size_t index=0; for(const auto& row:rows) body[index++]=menuJson(row);
    return crow::response(body);
}

crow::response PartnerController::createMenuItem(const crow::request& request,int restaurantId) {
    const auto actor=actorId(request); if(!actor) return error(401,"A valid bearer token is required");
    const auto json=crow::json::load(request.body);
    if(!json||!json.has("name")||!json.has("pricePaise")||!json.has("dietType"))
        return error(400,"Name, pricePaise and dietType are required");
    PartnerMenuItemRecord value; value.restaurantId=restaurantId; value.name=json["name"].s();
    value.description=json.has("description")?std::string(json["description"].s()):"";
    value.pricePaise=json["pricePaise"].i(); value.dietType=json["dietType"].s();
    value.available=!json.has("available")||json["available"].b();
    if(!partner::validName(value.name)||value.description.size()>500||
        !partner::validPricePaise(value.pricePaise)||!validDiet(value.dietType))
        return error(422,"Menu item details are invalid");
    const auto result=m_repository.createMenuItem(*actor,value,correlationId(request));
    if(result!=PartnerWriteResult::Ok) return writeResponse(result,"",201);
    auto body=menuJson(value); body["success"]=true; return crow::response(201,body);
}

crow::response PartnerController::updateMenuItem(const crow::request& request,
    int restaurantId,int itemId) {
    const auto actor=actorId(request); if(!actor) return error(401,"A valid bearer token is required");
    const auto json=crow::json::load(request.body);
    if(!json||!json.has("version")||!json.has("name")||!json.has("pricePaise")||
        !json.has("dietType")) return error(400,"Version and complete item details are required");
    PartnerMenuItemRecord value; value.id=itemId; value.restaurantId=restaurantId;
    value.name=json["name"].s(); value.description=json.has("description")?
        std::string(json["description"].s()):""; value.pricePaise=json["pricePaise"].i();
    value.dietType=json["dietType"].s(); value.available=!json.has("available")||json["available"].b();
    if(!partner::validName(value.name)||value.description.size()>500||
        !partner::validPricePaise(value.pricePaise)||!validDiet(value.dietType))
        return error(422,"Menu item details are invalid");
    const auto result=m_repository.updateMenuItem(*actor,value,json["version"].i(),correlationId(request));
    if(result!=PartnerWriteResult::Ok) return writeResponse(result,"");
    auto body=menuJson(value); body["success"]=true; return crow::response(body);
}

crow::response PartnerController::deleteMenuItem(const crow::request& request,
    int restaurantId,int itemId) {
    const auto actor=actorId(request); if(!actor) return error(401,"A valid bearer token is required");
    return writeResponse(m_repository.deleteMenuItem(*actor,restaurantId,itemId,
        correlationId(request)),"Menu item removed");
}

crow::response PartnerController::listAudit(const crow::request& request,int restaurantId) {
    const auto actor=actorId(request); if(!actor) return error(401,"A valid bearer token is required");
    if(!m_repository.getRestaurant(*actor,restaurantId)) return error(404,"Restaurant not found");
    const auto rows=m_repository.listAudit(*actor,restaurantId,50);
    crow::json::wvalue body(crow::json::wvalue::list{});
    std::size_t index=0; for(const auto& row:rows){body[index]["id"]=row.id;
        body[index]["actorUserId"]=row.actorUserId; body[index]["action"]=row.action;
        body[index]["resourceType"]=row.resourceType; body[index]["resourceId"]=row.resourceId;
        body[index]["result"]=row.result; body[index]["correlationId"]=row.correlationId;
        body[index]["createdEpoch"]=row.createdEpoch; ++index;}
    return crow::response(body);
}
