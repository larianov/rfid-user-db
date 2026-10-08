#pragma once
#include "list_u.hpp"
#include "pico/flash.h"
#include "hardware/flash.h"
#include <cstdint>
#include <etl/unordered_map.h>

inline constexpr uint starting_memory_adddress_mcu = XIP_BASE + PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE;
inline constexpr uint starting_memory_adddress_flash = starting_memory_adddress_mcu & 0xffffff;


struct memory_alligning{
    uint memory_addr = starting_memory_adddress_flash;
    uint8_t data[4096];
    std::size_t size{};
};


bool write_save(list_users &lu);
bool reader_after_wc(list_users &lu);


