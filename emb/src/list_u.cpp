#include "etl/unordered_map.h"
#include <cstdint>
#include <etl/optional.h>
#include "list_u.hpp"
list_users::list_users(uint64_t admin_uid, etl::string_view admin_name) : count_of_admins(1){
     list_of_users.try_emplace(admin_uid, user{admin_name, status::ADMIN});
}

insert_result list_users::insert_user(uint64_t uid, etl::string_view name, status status_of_user){
    if (list_of_users.full()) {
        return insert_result::full;
    }
    if (name.size() >= 15) {
        return insert_result::name_too_long;
    }
    auto [it, ins] = list_of_users.try_emplace(uid, user(name, status_of_user));
    if (!ins) {
        return insert_result::duplicate_uid;
    }
    if (status_of_user == status::ADMIN) {
        count_of_admins++;
    }
    return insert_result::ok;
}

bool list_users::remove_user(uint64_t uid){
    auto user = list_of_users.find(uid);
    if (user == list_of_users.end()) {
        return false;
    }
    if (count_of_admins == 1 && user->second.sou == status::ADMIN) {
        return false;
    }
    if (user->second.sou == status::ADMIN) {
        count_of_admins--;
    }
    list_of_users.erase(user);
    return true;
}

bool list_users::change_status(uint64_t uid, status new_status){
    auto user = list_of_users.find(uid);
    if (user == list_of_users.end()) {
        return false;
    }
    if (count_of_admins == 1 && user->second.sou == status::ADMIN) {
        return false;
    }
    if (new_status == status::ADMIN && user->second.sou != status::ADMIN) {
        count_of_admins++;
    }
    else if (new_status != status::ADMIN && user->second.sou == status::ADMIN) {
        count_of_admins--;
    }
    user->second.sou = new_status;
    return true;
}


std::size_t list_users::avialiable() const{
    return list_of_users.available();
}

etl::optional<etl::reference_wrapper<const user>> list_users::at(uint64_t key) const{
    auto it = list_of_users.find(key);
    if (it == list_of_users.end()) {
        return etl::nullopt;
    }
    return etl::cref(it->second);
}

const etl::unordered_map<uint64_t, user, size_of_map>& list_users::list(){return list_of_users;}
