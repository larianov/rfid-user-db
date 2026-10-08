#pragma once
#include <cstddef>
#include <cstdint>
#include <etl/optional.h>
#include <etl/string_view.h>
#include <functional>
#include "etl/string.h"
#include "etl/unordered_map.h"

enum class status: uint8_t{
    ADMIN = 2,
    USER = 1,
    BLACK_LIST = 0,
};

struct user{
    etl::string<16> Name;
    status sou;
    user(etl::string_view n, status s) : Name(n), sou(s) {}
    
};

enum class insert_result : uint8_t { ok, full, duplicate_uid, name_too_long, stat_error };

inline constexpr uint16_t size_of_map{100};

class list_users{
private:
    etl::unordered_map<uint64_t, user, size_of_map> list_of_users;
    uint16_t count_of_admins{};
public:
    list_users(uint64_t admin_uid, etl::string_view admin_name);
    insert_result insert_user(uint64_t uid, etl::string_view name, status status_of_user); // true -- user added, false - database is full
    bool remove_user(uint64_t uid); // true - user removed, false - user not found or user is last admin
    bool change_status(uint64_t uid, status new_status); // true - succsess with changing status, false - user not found or User is last Admin   
    [[nodiscard]]std::size_t avialiable() const;
    list_users(const list_users&) = delete;
    list_users& operator=(const list_users&) = delete;
    etl::optional<etl::reference_wrapper<const user>> at(uint64_t key) const;
    [[nodiscard]]const etl::unordered_map<uint64_t, user, size_of_map>& list();
};