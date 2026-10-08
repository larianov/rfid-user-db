#include "memory_map.hpp"
#include <cstdint>
#include <cstring>
#include <etl/char_traits.h>
#include <etl/string_view.h>
#include <pico/error.h>
#include <pico/flash.h>
#include "etl/crc32.h"
#include "list_u.hpp"

static void helper_writer(void* ptr){
    auto val = static_cast<memory_alligning*>(ptr);
    flash_range_program(val->memory_addr, val->data, val->size);
}
static void helper_eraser(void*ptr){
    auto val = static_cast<memory_alligning*>(ptr);
    flash_range_erase(val->memory_addr, val->size);
}


static memory_alligning memory_aligner{};


bool write_save(list_users &lu){
    memset(memory_aligner.data, 0x00, 4096);
    memory_aligner.memory_addr = starting_memory_adddress_flash;
    memory_aligner.size = 1;
    memory_aligner.data[0] = lu.list().size();
    for (; memory_aligner.size < 8; memory_aligner.size++) {
        memory_aligner.data[memory_aligner.size] = 0;
    }
    for (const auto &v : lu.list()) {
        memcpy(memory_aligner.data+memory_aligner.size, &v.first, sizeof(v.first));
        memory_aligner.size+=sizeof(uint64_t) / sizeof(uint8_t);
        uint8_t arr[16]{};
        memcpy(arr, v.second.Name.data(), v.second.Name.size());
        memcpy(memory_aligner.data+memory_aligner.size, arr, 16);
        memory_aligner.size+=16;
        memory_aligner.data[memory_aligner.size++] = static_cast<uint8_t>(v.second.sou);
        for (uint8_t i{1}; i < 8; i++) {
            memory_aligner.data[memory_aligner.size++] = 0x00;
        }
    }
    etl::crc32 d;
    d.add(memory_aligner.data, memory_aligner.data+memory_aligner.size);
    auto val = d.value();
    memcpy(memory_aligner.data+memory_aligner.size, &val, sizeof(val));
    
    memory_aligner.size = 4096;
    int err = flash_safe_execute(helper_eraser, &memory_aligner, 100);
    if (err != PICO_OK) {
        return false;
    }
    err = flash_safe_execute(helper_writer, &memory_aligner, 100);
    if (err != PICO_OK) {
        return false;
    }
    return true;
}

bool reader_after_wc(list_users &lu){
    auto *ptr = reinterpret_cast<const uint8_t *>(starting_memory_adddress_mcu);
    uint8_t ammount_of_users = *ptr;
    if (ammount_of_users == 0xff || ammount_of_users == 0x00 || ammount_of_users > lu.list().max_size()) return false;
    etl::crc32 d;
    auto *crc_dsz = ptr+8+(ammount_of_users*32);
    d.add(ptr, crc_dsz);
    auto value = d.value();
    if(memcmp(&value, crc_dsz, 4) != 0) return false;
    ptr+=8;
    for(; ptr < crc_dsz;){
        uint64_t uid{};
        memcpy(&uid, ptr, sizeof(uid));
        ptr+=8;
        etl::string<16> name{reinterpret_cast<const char *>(ptr), etl::strlen(reinterpret_cast<const char *>(ptr), 16)};
        ptr+=16;
        auto status_of_User = static_cast<enum status>(*ptr);
        ptr+=8;
        lu.insert_user(uid, name, status_of_User);
    }
    return true;
}