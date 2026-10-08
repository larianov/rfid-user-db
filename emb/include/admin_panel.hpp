#pragma once
#include "rc522/card_reader.hpp"
#include "list_u.hpp"
#include <cstdint>
#include <etl/string.h>
#include <pico/types.h>
#include "memory_map.hpp"

inline uint64_t uid_converter(const rc522::Uid &uid){
    uint64_t res{};
    for (uint64_t i{}; i < uid.size; i++) {
        res = (res << 8) | uid.bytes[i];
    }
    return res;
}



class admin_session{
private:
    rc522::CardReader &cr_;
    list_users &lu_;
    uint32_t time_constant = 5 * 60 * 1000;
    insert_result insert(etl::string<42> &str);
    insert_result change(etl::string<42> &str);
    insert_result remove(etl::string<42> &str);
    void list();
    absolute_time_t deadline{};
public:
    admin_session(rc522::CardReader &cr, list_users &lu);
    void run_session();
};

// insert 05050505050505