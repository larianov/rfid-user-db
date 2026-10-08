include(FetchContent)

set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
FetchContent_Declare(etl
    GIT_REPOSITORY https://github.com/ETLCPP/etl.git
    GIT_TAG        20.48.1
    GIT_SHALLOW    TRUE
    SYSTEM
)
FetchContent_MakeAvailable(etl)
unset(CMAKE_POLICY_VERSION_MINIMUM)


FetchContent_Declare(rc522
    GIT_REPOSITORY https://github.com/larianov/mfrc522-nonblocking.git
    GIT_TAG        main          
    GIT_SHALLOW    TRUE
    SYSTEM
)
FetchContent_MakeAvailable(rc522)

set(CARD_ANL_SOURCES
    src/main.cpp
    src/list_u.cpp
    src/admin_panel.cpp
    src/memory_map.cpp
)

add_executable(card_anl ${CARD_ANL_SOURCES})

target_include_directories(card_anl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

pico_enable_stdio_uart(card_anl 0)
pico_enable_stdio_usb(card_anl 1)

target_link_libraries(card_anl PRIVATE
    pico_stdlib
    hardware_dma
    pico_flash
    etl::etl
    rc522_pico
)

card_setup_firmware(card_anl ${CARD_ANL_SOURCES})