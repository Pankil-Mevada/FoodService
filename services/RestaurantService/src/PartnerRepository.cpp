#include "PartnerRepository.h"
#include "PartnerAccessPolicy.h"

#include <algorithm>
#include <chrono>
#include <sqlite3.h>

namespace {
long long nowEpoch() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
std::string text(sqlite3_stmt* statement, int column) {
    const auto* value = sqlite3_column_text(statement, column);
    return value ? reinterpret_cast<const char*>(value) : "";
}
partner::Role roleValue(const std::string& role) {
    if (role == "OWNER") return partner::Role::Owner;
    if (role == "MANAGER") return partner::Role::Manager;
    if (role == "STAFF") return partner::Role::Staff;
    return partner::Role::Unknown;
}
bool editableStatus(const std::string& status) {
    return status == "DRAFT" || status == "REJECTED";
}

class Transaction {
public:
    explicit Transaction(Database& database)
        : m_database(database), m_active(database.execute("BEGIN IMMEDIATE;")) {}

    ~Transaction() {
        if (m_active) m_database.execute("ROLLBACK;");
    }

    bool started() const { return m_active; }

    bool commit() {
        if (!m_active || !m_database.execute("COMMIT;")) return false;
        m_active = false;
        return true;
    }

private:
    Database& m_database;
    bool m_active;
};
}

PartnerRepository::PartnerRepository(Database& database) : m_database(database) {
    m_ready = ensureSchema();
}

bool PartnerRepository::ensureRestaurantColumn(const std::string& name,
    const std::string& definition) {
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(m_database.connection(), "PRAGMA table_info(restaurants);",
            -1, &statement, nullptr) != SQLITE_OK) return false;
    bool found = false;
    while (sqlite3_step(statement) == SQLITE_ROW) {
        if (text(statement, 1) == name) { found = true; break; }
    }
    sqlite3_finalize(statement);
    return found || m_database.execute("ALTER TABLE restaurants ADD COLUMN " +
        name + " " + definition + ";");
}

bool PartnerRepository::ensureSchema() {
    std::lock_guard<std::recursive_mutex> lock(m_database.mutex());
    if (!ensureRestaurantColumn("onboarding_status",
            "TEXT NOT NULL DEFAULT 'APPROVED'")) return false;
    if (!ensureRestaurantColumn("version", "INTEGER NOT NULL DEFAULT 1")) return false;
    if (!ensureRestaurantColumn("updated_epoch", "INTEGER NOT NULL DEFAULT 0")) return false;
    return m_database.execute(R"(
        CREATE TABLE IF NOT EXISTS restaurant_partners (
            restaurant_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            role TEXT NOT NULL CHECK(role IN ('OWNER','MANAGER','STAFF')),
            membership_status TEXT NOT NULL DEFAULT 'ACTIVE'
                CHECK(membership_status IN ('INVITED','ACTIVE','REVOKED')),
            created_epoch INTEGER NOT NULL,
            updated_epoch INTEGER NOT NULL,
            PRIMARY KEY(restaurant_id,user_id),
            FOREIGN KEY(restaurant_id) REFERENCES restaurants(id) ON DELETE CASCADE
        );
        CREATE INDEX IF NOT EXISTS idx_restaurant_partners_user
            ON restaurant_partners(user_id,membership_status,restaurant_id);
        CREATE TABLE IF NOT EXISTS partner_menu_items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            restaurant_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            description TEXT NOT NULL DEFAULT '',
            price_paise INTEGER NOT NULL CHECK(price_paise BETWEEN 0 AND 100000000),
            diet_type TEXT NOT NULL CHECK(diet_type IN ('VEG','NON_VEG','VEGAN')),
            available INTEGER NOT NULL DEFAULT 1 CHECK(available IN (0,1)),
            version INTEGER NOT NULL DEFAULT 1,
            created_epoch INTEGER NOT NULL,
            updated_epoch INTEGER NOT NULL,
            FOREIGN KEY(restaurant_id) REFERENCES restaurants(id) ON DELETE CASCADE
        );
        CREATE INDEX IF NOT EXISTS idx_partner_menu_restaurant
            ON partner_menu_items(restaurant_id,id);
        CREATE TABLE IF NOT EXISTS partner_audit_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            restaurant_id INTEGER NOT NULL,
            actor_user_id INTEGER NOT NULL,
            action TEXT NOT NULL,
            resource_type TEXT NOT NULL,
            resource_id INTEGER NOT NULL,
            result TEXT NOT NULL,
            correlation_id TEXT NOT NULL DEFAULT '',
            created_epoch INTEGER NOT NULL,
            FOREIGN KEY(restaurant_id) REFERENCES restaurants(id) ON DELETE CASCADE
        );
        CREATE INDEX IF NOT EXISTS idx_partner_audit_restaurant
            ON partner_audit_events(restaurant_id,id DESC);
    )");
}

std::optional<std::string> PartnerRepository::activeRole(int userId, int restaurantId) {
    sqlite3_stmt* statement = nullptr;
    const char* sql = "SELECT role FROM restaurant_partners "
        "WHERE user_id=? AND restaurant_id=? AND membership_status='ACTIVE';";
    if (sqlite3_prepare_v2(m_database.connection(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return std::nullopt;
    sqlite3_bind_int(statement, 1, userId);
    sqlite3_bind_int(statement, 2, restaurantId);
    std::optional<std::string> result;
    if (sqlite3_step(statement) == SQLITE_ROW) result = text(statement, 0);
    sqlite3_finalize(statement);
    return result;
}

std::optional<std::string> PartnerRepository::restaurantStatus(int restaurantId) {
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(m_database.connection(),
            "SELECT onboarding_status FROM restaurants WHERE id=?;", -1,
            &statement, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_int(statement, 1, restaurantId);
    std::optional<std::string> result;
    if (sqlite3_step(statement) == SQLITE_ROW) result = text(statement, 0);
    sqlite3_finalize(statement);
    return result;
}

bool PartnerRepository::appendAudit(int actorUserId, int restaurantId,
    const std::string& action, const std::string& resourceType, int resourceId,
    const std::string& result, const std::string& correlationId) {
    sqlite3_stmt* statement = nullptr;
    const char* sql = "INSERT INTO partner_audit_events"
        "(restaurant_id,actor_user_id,action,resource_type,resource_id,result,"
        "correlation_id,created_epoch) VALUES(?,?,?,?,?,?,?,?);";
    if (sqlite3_prepare_v2(m_database.connection(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_int(statement, 1, restaurantId);
    sqlite3_bind_int(statement, 2, actorUserId);
    sqlite3_bind_text(statement, 3, action.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 4, resourceType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 5, resourceId);
    sqlite3_bind_text(statement, 6, result.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 7, correlationId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(statement, 8, nowEpoch());
    const int rc = sqlite3_step(statement);
    sqlite3_finalize(statement);
    return rc == SQLITE_DONE;
}

PartnerWriteResult PartnerRepository::createRestaurant(int userId,
    const PartnerRestaurantRecord& value, const std::string& correlationId,
    int& restaurantId) {
    std::lock_guard<std::recursive_mutex> lock(m_database.mutex());
    if (!m_ready || userId <= 0) return PartnerWriteResult::Error;
    if (!m_database.execute("BEGIN IMMEDIATE;")) return PartnerWriteResult::Error;
    sqlite3_stmt* statement = nullptr;
    const char* sql = "INSERT INTO restaurants(name,address,phone,rating,latitude,"
        "longitude,delivery_radius_km,image_url,delivery_polygon,base_delivery_fee,"
        "per_km_fee,preparation_minutes,onboarding_status,version,updated_epoch)"
        " VALUES(?,?,?,0,?,?,?,?,'',?,?,?,'DRAFT',1,?);";
    if (sqlite3_prepare_v2(m_database.connection(), sql, -1, &statement, nullptr) != SQLITE_OK) {
        m_database.execute("ROLLBACK;"); return PartnerWriteResult::Error;
    }
    sqlite3_bind_text(statement, 1, value.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, value.address.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, value.phone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(statement, 4, value.latitude);
    sqlite3_bind_double(statement, 5, value.longitude);
    sqlite3_bind_double(statement, 6, value.deliveryRadiusKm);
    sqlite3_bind_text(statement, 7, value.imageUrl.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(statement, 8, value.baseDeliveryFee);
    sqlite3_bind_double(statement, 9, value.perKmFee);
    sqlite3_bind_int(statement, 10, value.preparationMinutes);
    sqlite3_bind_int64(statement, 11, nowEpoch());
    const int restaurantRc = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (restaurantRc != SQLITE_DONE) {
        m_database.execute("ROLLBACK;"); return PartnerWriteResult::Error;
    }
    restaurantId = static_cast<int>(sqlite3_last_insert_rowid(m_database.connection()));
    const char* membershipSql = "INSERT INTO restaurant_partners"
        "(restaurant_id,user_id,role,membership_status,created_epoch,updated_epoch)"
        " VALUES(?,?,'OWNER','ACTIVE',?,?);";
    if (sqlite3_prepare_v2(m_database.connection(), membershipSql, -1,
            &statement, nullptr) != SQLITE_OK) {
        m_database.execute("ROLLBACK;"); return PartnerWriteResult::Error;
    }
    const auto now = nowEpoch();
    sqlite3_bind_int(statement, 1, restaurantId);
    sqlite3_bind_int(statement, 2, userId);
    sqlite3_bind_int64(statement, 3, now);
    sqlite3_bind_int64(statement, 4, now);
    const int membershipRc = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (membershipRc != SQLITE_DONE ||
        !appendAudit(userId, restaurantId, "RESTAURANT_CREATED", "RESTAURANT",
            restaurantId, "SUCCESS", correlationId) ||
        !m_database.execute("COMMIT;")) {
        m_database.execute("ROLLBACK;"); return PartnerWriteResult::Error;
    }
    return PartnerWriteResult::Ok;
}

std::vector<PartnerRestaurantRecord> PartnerRepository::listRestaurants(int userId) {
    std::lock_guard<std::recursive_mutex> lock(m_database.mutex());
    std::vector<PartnerRestaurantRecord> result;
    sqlite3_stmt* statement = nullptr;
    const char* sql = "SELECT r.id,r.name,r.address,r.phone,r.latitude,r.longitude,"
        "r.delivery_radius_km,r.image_url,r.base_delivery_fee,r.per_km_fee,"
        "r.preparation_minutes,r.onboarding_status,p.role,r.version "
        "FROM restaurants r JOIN restaurant_partners p ON p.restaurant_id=r.id "
        "WHERE p.user_id=? AND p.membership_status='ACTIVE' ORDER BY r.id DESC;";
    if (sqlite3_prepare_v2(m_database.connection(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return result;
    sqlite3_bind_int(statement, 1, userId);
    while (sqlite3_step(statement) == SQLITE_ROW) {
        PartnerRestaurantRecord row;
        row.id=sqlite3_column_int(statement,0); row.name=text(statement,1);
        row.address=text(statement,2); row.phone=text(statement,3);
        row.latitude=sqlite3_column_double(statement,4);
        row.longitude=sqlite3_column_double(statement,5);
        row.deliveryRadiusKm=sqlite3_column_double(statement,6);
        row.imageUrl=text(statement,7); row.baseDeliveryFee=sqlite3_column_double(statement,8);
        row.perKmFee=sqlite3_column_double(statement,9);
        row.preparationMinutes=sqlite3_column_int(statement,10);
        row.status=text(statement,11); row.role=text(statement,12);
        row.version=sqlite3_column_int(statement,13); result.push_back(row);
    }
    sqlite3_finalize(statement);
    return result;
}

std::optional<PartnerRestaurantRecord> PartnerRepository::getRestaurant(
    int userId, int restaurantId) {
    const auto rows = listRestaurants(userId);
    const auto found = std::find_if(rows.begin(), rows.end(),
        [restaurantId](const auto& row){ return row.id == restaurantId; });
    return found == rows.end() ? std::nullopt :
        std::optional<PartnerRestaurantRecord>(*found);
}

PartnerWriteResult PartnerRepository::updateRestaurant(int userId,
    PartnerRestaurantRecord& value, int expectedVersion,
    const std::string& correlationId) {
    std::lock_guard<std::recursive_mutex> lock(m_database.mutex());
    Transaction transaction(m_database);
    if (!transaction.started()) return PartnerWriteResult::Error;
    const auto role=activeRole(userId,value.id);
    if(!role) return PartnerWriteResult::NotFound;
    if(!partner::allowed(roleValue(*role),partner::Permission::EditRestaurant))
        return PartnerWriteResult::Forbidden;
    const auto status=restaurantStatus(value.id);
    if(!status) return PartnerWriteResult::NotFound;
    if(!editableStatus(*status)) return PartnerWriteResult::InvalidState;
    sqlite3_stmt* statement=nullptr;
    const char* sql="UPDATE restaurants SET name=?,address=?,phone=?,latitude=?,"
        "longitude=?,delivery_radius_km=?,image_url=?,base_delivery_fee=?,"
        "per_km_fee=?,preparation_minutes=?,version=version+1,updated_epoch=? "
        "WHERE id=? AND version=?;";
    if(sqlite3_prepare_v2(m_database.connection(),sql,-1,&statement,nullptr)!=SQLITE_OK)
        return PartnerWriteResult::Error;
    sqlite3_bind_text(statement,1,value.name.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(statement,2,value.address.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(statement,3,value.phone.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_double(statement,4,value.latitude); sqlite3_bind_double(statement,5,value.longitude);
    sqlite3_bind_double(statement,6,value.deliveryRadiusKm);
    sqlite3_bind_text(statement,7,value.imageUrl.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_double(statement,8,value.baseDeliveryFee); sqlite3_bind_double(statement,9,value.perKmFee);
    sqlite3_bind_int(statement,10,value.preparationMinutes); sqlite3_bind_int64(statement,11,nowEpoch());
    sqlite3_bind_int(statement,12,value.id); sqlite3_bind_int(statement,13,expectedVersion);
    const int rc=sqlite3_step(statement); sqlite3_finalize(statement);
    if(rc!=SQLITE_DONE) return PartnerWriteResult::Error;
    if(sqlite3_changes(m_database.connection())==0) return PartnerWriteResult::Conflict;
    value.version=expectedVersion+1;
    if (!appendAudit(userId,value.id,"RESTAURANT_UPDATED","RESTAURANT",value.id,
            "SUCCESS",correlationId) || !transaction.commit())
        return PartnerWriteResult::Error;
    return PartnerWriteResult::Ok;
}

PartnerWriteResult PartnerRepository::submitRestaurant(int userId,int restaurantId,
    int expectedVersion,const std::string& correlationId,int& newVersion) {
    std::lock_guard<std::recursive_mutex> lock(m_database.mutex());
    Transaction transaction(m_database);
    if (!transaction.started()) return PartnerWriteResult::Error;
    const auto role=activeRole(userId,restaurantId);
    if(!role) return PartnerWriteResult::NotFound;
    if(!partner::allowed(roleValue(*role),partner::Permission::Submit))
        return PartnerWriteResult::Forbidden;
    const auto status=restaurantStatus(restaurantId);
    if(!status) return PartnerWriteResult::NotFound;
    if(!editableStatus(*status)) return PartnerWriteResult::InvalidState;
    sqlite3_stmt* statement=nullptr;
    if(sqlite3_prepare_v2(m_database.connection(),
        "SELECT COUNT(*) FROM partner_menu_items WHERE restaurant_id=?;",-1,
        &statement,nullptr)!=SQLITE_OK) return PartnerWriteResult::Error;
    sqlite3_bind_int(statement,1,restaurantId);
    const bool hasMenu=sqlite3_step(statement)==SQLITE_ROW&&sqlite3_column_int(statement,0)>0;
    sqlite3_finalize(statement);
    if(!hasMenu) return PartnerWriteResult::Invalid;
    if(sqlite3_prepare_v2(m_database.connection(),
        "UPDATE restaurants SET onboarding_status='PENDING_REVIEW',version=version+1,"
        "updated_epoch=? WHERE id=? AND version=?;",-1,&statement,nullptr)!=SQLITE_OK)
        return PartnerWriteResult::Error;
    sqlite3_bind_int64(statement,1,nowEpoch()); sqlite3_bind_int(statement,2,restaurantId);
    sqlite3_bind_int(statement,3,expectedVersion);
    const int rc=sqlite3_step(statement); sqlite3_finalize(statement);
    if(rc!=SQLITE_DONE) return PartnerWriteResult::Error;
    if(sqlite3_changes(m_database.connection())==0) return PartnerWriteResult::Conflict;
    newVersion=expectedVersion+1;
    if (!appendAudit(userId,restaurantId,"RESTAURANT_SUBMITTED","RESTAURANT",
            restaurantId,"SUCCESS",correlationId) || !transaction.commit())
        return PartnerWriteResult::Error;
    return PartnerWriteResult::Ok;
}

std::vector<PartnerMenuItemRecord> PartnerRepository::listMenuItems(
    int userId,int restaurantId) {
    std::lock_guard<std::recursive_mutex> lock(m_database.mutex());
    std::vector<PartnerMenuItemRecord> result;
    if(!activeRole(userId,restaurantId)) return result;
    sqlite3_stmt* statement=nullptr;
    if(sqlite3_prepare_v2(m_database.connection(),
        "SELECT id,restaurant_id,name,description,price_paise,diet_type,available,"
        "version FROM partner_menu_items WHERE restaurant_id=? ORDER BY id;",-1,
        &statement,nullptr)!=SQLITE_OK) return result;
    sqlite3_bind_int(statement,1,restaurantId);
    while(sqlite3_step(statement)==SQLITE_ROW){
        PartnerMenuItemRecord row; row.id=sqlite3_column_int(statement,0);
        row.restaurantId=sqlite3_column_int(statement,1); row.name=text(statement,2);
        row.description=text(statement,3); row.pricePaise=sqlite3_column_int64(statement,4);
        row.dietType=text(statement,5); row.available=sqlite3_column_int(statement,6)==1;
        row.version=sqlite3_column_int(statement,7); result.push_back(row);
    }
    sqlite3_finalize(statement); return result;
}

PartnerWriteResult PartnerRepository::createMenuItem(int userId,
    PartnerMenuItemRecord& value,const std::string& correlationId) {
    std::lock_guard<std::recursive_mutex> lock(m_database.mutex());
    Transaction transaction(m_database);
    if (!transaction.started()) return PartnerWriteResult::Error;
    const auto role=activeRole(userId,value.restaurantId);
    if(!role) return PartnerWriteResult::NotFound;
    if(!partner::allowed(roleValue(*role),partner::Permission::EditMenu))
        return PartnerWriteResult::Forbidden;
    const auto status=restaurantStatus(value.restaurantId);
    if(!status||!editableStatus(*status)) return PartnerWriteResult::InvalidState;
    sqlite3_stmt* statement=nullptr;
    const char* sql="INSERT INTO partner_menu_items(restaurant_id,name,description,"
        "price_paise,diet_type,available,version,created_epoch,updated_epoch)"
        " VALUES(?,?,?,?,?,?,1,?,?);";
    if(sqlite3_prepare_v2(m_database.connection(),sql,-1,&statement,nullptr)!=SQLITE_OK)
        return PartnerWriteResult::Error;
    sqlite3_bind_int(statement,1,value.restaurantId);
    sqlite3_bind_text(statement,2,value.name.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(statement,3,value.description.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_int64(statement,4,value.pricePaise);
    sqlite3_bind_text(statement,5,value.dietType.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_int(statement,6,value.available?1:0);
    const auto now=nowEpoch(); sqlite3_bind_int64(statement,7,now); sqlite3_bind_int64(statement,8,now);
    const int rc=sqlite3_step(statement); sqlite3_finalize(statement);
    if(rc!=SQLITE_DONE) return PartnerWriteResult::Error;
    value.id=static_cast<int>(sqlite3_last_insert_rowid(m_database.connection()));
    value.version=1;
    if (!appendAudit(userId,value.restaurantId,"MENU_ITEM_CREATED","MENU_ITEM",value.id,
            "SUCCESS",correlationId) || !transaction.commit())
        return PartnerWriteResult::Error;
    return PartnerWriteResult::Ok;
}

PartnerWriteResult PartnerRepository::updateMenuItem(int userId,
    PartnerMenuItemRecord& value,int expectedVersion,const std::string& correlationId) {
    std::lock_guard<std::recursive_mutex> lock(m_database.mutex());
    Transaction transaction(m_database);
    if (!transaction.started()) return PartnerWriteResult::Error;
    const auto role=activeRole(userId,value.restaurantId);
    if(!role) return PartnerWriteResult::NotFound;
    if(!partner::allowed(roleValue(*role),partner::Permission::EditMenu))
        return PartnerWriteResult::Forbidden;
    const auto status=restaurantStatus(value.restaurantId);
    if(!status||!editableStatus(*status)) return PartnerWriteResult::InvalidState;
    sqlite3_stmt* statement=nullptr;
    const char* sql="UPDATE partner_menu_items SET name=?,description=?,price_paise=?,"
        "diet_type=?,available=?,version=version+1,updated_epoch=? "
        "WHERE id=? AND restaurant_id=? AND version=?;";
    if(sqlite3_prepare_v2(m_database.connection(),sql,-1,&statement,nullptr)!=SQLITE_OK)
        return PartnerWriteResult::Error;
    sqlite3_bind_text(statement,1,value.name.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(statement,2,value.description.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_int64(statement,3,value.pricePaise);
    sqlite3_bind_text(statement,4,value.dietType.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_int(statement,5,value.available?1:0); sqlite3_bind_int64(statement,6,nowEpoch());
    sqlite3_bind_int(statement,7,value.id); sqlite3_bind_int(statement,8,value.restaurantId);
    sqlite3_bind_int(statement,9,expectedVersion);
    const int rc=sqlite3_step(statement); sqlite3_finalize(statement);
    if(rc!=SQLITE_DONE) return PartnerWriteResult::Error;
    if(sqlite3_changes(m_database.connection())==0) return PartnerWriteResult::Conflict;
    value.version=expectedVersion+1;
    if (!appendAudit(userId,value.restaurantId,"MENU_ITEM_UPDATED","MENU_ITEM",value.id,
            "SUCCESS",correlationId) || !transaction.commit())
        return PartnerWriteResult::Error;
    return PartnerWriteResult::Ok;
}

PartnerWriteResult PartnerRepository::deleteMenuItem(int userId,int restaurantId,
    int itemId,const std::string& correlationId) {
    std::lock_guard<std::recursive_mutex> lock(m_database.mutex());
    Transaction transaction(m_database);
    if (!transaction.started()) return PartnerWriteResult::Error;
    const auto role=activeRole(userId,restaurantId);
    if(!role) return PartnerWriteResult::NotFound;
    if(!partner::allowed(roleValue(*role),partner::Permission::EditMenu))
        return PartnerWriteResult::Forbidden;
    const auto status=restaurantStatus(restaurantId);
    if(!status||!editableStatus(*status)) return PartnerWriteResult::InvalidState;
    sqlite3_stmt* statement=nullptr;
    if(sqlite3_prepare_v2(m_database.connection(),
        "DELETE FROM partner_menu_items WHERE id=? AND restaurant_id=?;",-1,
        &statement,nullptr)!=SQLITE_OK) return PartnerWriteResult::Error;
    sqlite3_bind_int(statement,1,itemId); sqlite3_bind_int(statement,2,restaurantId);
    const int rc=sqlite3_step(statement); sqlite3_finalize(statement);
    if(rc!=SQLITE_DONE) return PartnerWriteResult::Error;
    if(sqlite3_changes(m_database.connection())==0) return PartnerWriteResult::NotFound;
    if (!appendAudit(userId,restaurantId,"MENU_ITEM_DELETED","MENU_ITEM",itemId,
            "SUCCESS",correlationId) || !transaction.commit())
        return PartnerWriteResult::Error;
    return PartnerWriteResult::Ok;
}

std::vector<PartnerAuditRecord> PartnerRepository::listAudit(
    int userId,int restaurantId,int limit) {
    std::lock_guard<std::recursive_mutex> lock(m_database.mutex());
    std::vector<PartnerAuditRecord> result;
    if(!activeRole(userId,restaurantId)) return result;
    limit=std::clamp(limit,1,100);
    sqlite3_stmt* statement=nullptr;
    if(sqlite3_prepare_v2(m_database.connection(),
        "SELECT id,actor_user_id,action,resource_type,resource_id,result,"
        "correlation_id,created_epoch FROM partner_audit_events "
        "WHERE restaurant_id=? ORDER BY id DESC LIMIT ?;",-1,&statement,nullptr)!=SQLITE_OK)
        return result;
    sqlite3_bind_int(statement,1,restaurantId); sqlite3_bind_int(statement,2,limit);
    while(sqlite3_step(statement)==SQLITE_ROW){
        PartnerAuditRecord row; row.id=sqlite3_column_int64(statement,0);
        row.actorUserId=sqlite3_column_int(statement,1); row.action=text(statement,2);
        row.resourceType=text(statement,3); row.resourceId=sqlite3_column_int(statement,4);
        row.result=text(statement,5); row.correlationId=text(statement,6);
        row.createdEpoch=sqlite3_column_int64(statement,7); result.push_back(row);
    }
    sqlite3_finalize(statement); return result;
}
