#include "admin_panel.hpp"
#include "list_u.hpp"
#include "memory_map.hpp"
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <etl/string_view.h>
#include <etl/vector.h>
#include <iostream>
#include <pico/error.h>
#include <etl/string.h>
#include <hardware/timer.h>
#include <pico/stdio.h>
#include <pico/stdio_usb.h>
#include <pico/time.h>
#include <rc522/enums.hpp>



etl::optional<rc522::Uid> get_uid_for_n_time(uint32_t us, rc522::CardReader &cr){
    uint32_t time_start = time_us_32();
    while (time_us_32() - time_start < us) {
        auto res = cr.start_uid_transaction();
        if (res != rc522::result_of_card::SUCC) return etl::nullopt;
        res = rc522::result_of_card::WAIT;
        while (res == rc522::result_of_card::WAIT) {
            res = cr.poll();
        }
        if (res == rc522::result_of_card::SUCC) return cr.uid();
    }
    return etl::nullopt;
}

static bool input_reader(etl::string<42>&string){
    auto ch = getchar_timeout_us(0);
    if (ch == PICO_ERROR_TIMEOUT) return false;    
    if (ch == 0x08 || ch == 0x7F) {
        if (!string.empty()) {
            string.pop_back();
            printf("\b \b");
        }
        else {
            printf("\a");
        }
        return false;
    }
    if(ch == '\r' || ch == '\n'){
        while (getchar_timeout_us(0) != PICO_ERROR_TIMEOUT); 
        printf("\n\r");
        return true;
    }
    else {
        if (string.full()) {
            while (getchar_timeout_us(0) != PICO_ERROR_TIMEOUT); 
            std::printf("\r\nBuffer is full, try again\r\n");
            std::printf("admin> ");
            string.clear();
            return false;
        }
        else {
            if (ch >= 0x20 && ch <= 0x7E) {
                printf("%c", ch);
                string.push_back(ch);
            }
            return false;
        }
    }
}


template <typename T>
bool parse_uint(etl::string_view s, T& out, int base) {
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out, base);
    return ec == std::errc{} && ptr == s.data() + s.size();
}

insert_result admin_session::insert(etl::string<42> &str){
    etl::vector<etl::string_view, 4>vector_tockens;
    etl::optional<etl::string_view> token;
    while ((token = etl::get_token(str, " ", token, true))) {
        if (vector_tockens.full()) {  break; }
        vector_tockens.push_back(*token);
    }
    if (vector_tockens.full()){
        printf("Parsing error\r\n");
        return insert_result::stat_error;
    }
    if (vector_tockens.size() < 3 || (vector_tockens[0] != "insert" && vector_tockens[0] != "i")){
        printf("Parsing error\r\n");
        return insert_result::stat_error;
    }
    printf("Please, place the card\r\n");
    auto res = get_uid_for_n_time(7*1000*1000, cr_);
    if (!res.has_value()) {
        printf("Card wasn't found\r\n");
        return insert_result::stat_error;
    }
    uint8_t status;
    if (!parse_uint(vector_tockens[2], status,10 ) || status > 2) {
        std::printf("Wrong status\r\n");
        return insert_result::stat_error;
    }
    auto insert_res = lu_.insert_user(uid_converter(res.value()), vector_tockens[1], static_cast<enum status>(status));
    if (insert_res == insert_result::full) {
        std::printf("Database is full\r\n");
        return insert_res;
    }
    else if (insert_res == insert_result::duplicate_uid) {
        std::printf("User is already in database\r\n");
        return insert_res;
    }
    else if (insert_res == insert_result::name_too_long) {
        std::printf("Name is too long\r\n");
        return insert_res;
    }
    else if (insert_res == insert_result::ok) {
        std::printf("Success\r\n");
        return insert_result::ok;
    }
    return insert_result::stat_error;
}


insert_result admin_session::remove(etl::string<42> &str){
    etl::vector<etl::string_view, 3>vector_tockens;
    etl::optional<etl::string_view> token;
    while ((token = etl::get_token(str, " ", token, true))) {
        if (vector_tockens.full()) {  break; }
        vector_tockens.push_back(*token);
    }
    if (vector_tockens.full()){
        printf("Parsing error\r\n");
        return insert_result::stat_error;
    }
    if (vector_tockens.empty() || (vector_tockens[0] != "remove" && vector_tockens[0] != "r")){
        printf("Parsing error\r\n");
        return insert_result::stat_error;
    }
    uint64_t buff{};
    if (vector_tockens.size() == 2) {
        if(!parse_uint(vector_tockens[1], buff, 10)){
            printf("Parsing error\r\n");
            return insert_result::stat_error;
        }
    }
    else {
        printf("Please, place the card\r\n");
        auto res = get_uid_for_n_time(7*1000*1000, cr_);
        if (!res.has_value()) {
            printf("Card wasn't found\r\n");
            return insert_result::stat_error;
        }
        buff = uid_converter(res.value());
    }
    auto res = lu_.remove_user(buff);
    if (res){
        std::printf("Success\r\n");
        return insert_result::ok;
    }
    else {
        std::printf("Operation not possible\r\n");
        return insert_result::stat_error;
    }
}

insert_result admin_session::change(etl::string<42> &str){
    etl::vector<etl::string_view, 4>vector_tockens;
    etl::optional<etl::string_view> token;
    while ((token = etl::get_token(str, " ", token, true))) {
        if (vector_tockens.full()) {  break; }
        vector_tockens.push_back(*token);
    }
    if (vector_tockens.full()){
        printf("Parsing error\r\n");
        return insert_result::stat_error;
    }
    if (vector_tockens.size() < 2 || (vector_tockens[0] != "change" && vector_tockens[0] != "c")){
        printf("Parsing error\r\n");
        return insert_result::stat_error;
    }
    if (vector_tockens.size() == 3) {
        uint64_t uid{};
        if(!parse_uint(vector_tockens[2], uid, 10)){
            printf("Parsing error\r\n");
            return insert_result::stat_error;
        }
        uint8_t status{3};
        if(!parse_uint(vector_tockens[1], status, 10) || status > 2){
            printf("Parsing error\r\n");
            return insert_result::stat_error;
        }
        auto res = lu_.change_status(uid, static_cast<enum status>(status));
        if (res){
            std::printf("Success\r\n");
            return insert_result::ok;
        }
        else {
            std::printf("Operation not possible\r\n");
            return insert_result::stat_error;
        }
    }
    else {
        printf("Please, place the card\r\n");
        auto res = get_uid_for_n_time(7*1000*1000, cr_);
        if (!res.has_value()) {
            printf("Card wasn't found\r\n");
            return insert_result::stat_error;
        }
        uint8_t status{3};
        if(!parse_uint(vector_tockens[1], status, 10) || status > 2){
            printf("Parsing error\r\n");
            return insert_result::stat_error;
        }
        auto result = lu_.change_status(uid_converter(res.value()), static_cast<enum status>(status));
        if (result){
            std::printf("Success\r\n");
            return insert_result::ok;
        }
        else {
            std::printf("Operation not possible\r\n");
            return insert_result::stat_error;
        }
    }
}


admin_session::admin_session(rc522::CardReader &cr, list_users &lu) : cr_(cr), lu_(lu) {}
void admin_session::run_session(){
    deadline = make_timeout_time_ms(time_constant);
    etl::string<42> string{};
    while (!stdio_usb_connected() && !time_reached(deadline)) {
        sleep_us(20);
    }
    if (time_reached(deadline)){
        printf("CON\r\n\r\n");
        return;
    }
    sleep_ms(100);
    while (getchar_timeout_us(0) != PICO_ERROR_TIMEOUT); 
    std::printf("admin> ");
    while (!time_reached(deadline)) {
        if (!input_reader(string)) continue;
        if (string.starts_with("i")) {
            insert(string);
        }
        else if (string.starts_with("r")) {
            remove(string);
        }
        else if (string.starts_with("c")) {
            change(string);
        }
        else if(string == "list" || string == "l"){
            auto res = lu_.list();
            for (auto &v : res) {
                printf("User id: %llu, User name: %s, User status: %u\r\n", v.first, v.second.Name.c_str(), static_cast<uint8_t>(v.second.sou));
            }
        }
        else if (string == "help" || string == "h") {
            printf("insert <name> <status> - insert new user\r\nchange <status> (id) - change status of existing user\r\nremove (id) - remove existing user\r\nlist - list existing users\r\nq - to quit\r\nstatuses: (0 - block, 1 - user, 2 - admin)\r\nsave - for saving");
        }
        else if (string == "q") {
            printf("CON\r\n\r\n");
            return;
        }
        else if (string == "s" || string == "save") {
            if (write_save(lu_)) {
                printf("Success backloged\r\n");
            }
            else {
                printf("Unsuccessfull, try again!");
            }
        }
        else {
            printf("Unknown command, check help\r\n");
        }
        string.clear();
        printf("admin> ");
    }
    printf("CON\r\n\r\n");
}