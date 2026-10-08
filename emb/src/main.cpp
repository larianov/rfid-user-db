#include "admin_panel.hpp"
#include "pico/stdlib.h"
#include <boards/pico.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <hardware/gpio.h>
#include <hardware/regs/addressmap.h>
#include <hardware/spi.h>
#include <hardware/timer.h>
#include <pico/error.h>
#include <pico/platform/common.h>
#include <pico/stdio.h>
#include <pico/time.h>
#include <pico/types.h>
#include <rc522/enums.hpp>
#include "memory_map.hpp"
#include "list_u.hpp"
#include "rc522/platform/pico/spi_transport.hpp"
#include "rc522/card_reader.hpp"
#include "rc522/rc522.hpp"
#include "hardware/sync.h"

constexpr uint64_t ADMIN_KEY = 0x572A9331;
static constexpr uint8_t blocked_led = 13;
static constexpr uint8_t user_led = 14;
static constexpr uint8_t admin_led = 15;

static constexpr uint8_t irq_pin{6};

static volatile bool needs_to_poll{true};
void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == irq_pin && (events & GPIO_IRQ_EDGE_FALL)) {
        needs_to_poll = true;
    }
}


int main(){
    stdio_init_all();
    sleep_ms(2000);
    
    spi_init(spi0, 5'000'000);
    gpio_set_function(2, GPIO_FUNC_SPI);
    gpio_set_function(3, GPIO_FUNC_SPI);
    gpio_set_function(4, GPIO_FUNC_SPI);
    gpio_init(5);
    gpio_set_dir(5, GPIO_OUT);
    gpio_put(5, 1);
    for (uint8_t i{blocked_led}; i <= admin_led; i++) {
        gpio_init(i);
        gpio_set_dir(i, true);
    }
    
    gpio_init(irq_pin);
    gpio_set_dir(irq_pin, GPIO_IN);
    gpio_set_irq_enabled_with_callback(irq_pin, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);



    rc522::PicoTransport transport(5, spi0);
    rc522::Rc522 ic(transport);
    if (ic.init(true) != rc522::result_of_op::SUCC) {
        printf("RC522 init failed, check wiring\n");
    } else {
        printf("RC522 init OK\n");
    }
    rc522::CardReader cr(ic);
    
    
    list_users lu{ADMIN_KEY, "admin"};
    if (reader_after_wc(lu)) printf("SUCC ON RECOVERING\r\n");
    else printf("RECOVERING NOT FOUND\r\n");
    admin_session ses(cr, lu);
    
    while (true) {
        cr.start_uid_transaction(rc522::WAKING_CARD_UP_FOR_UID::REQA);
        needs_to_poll = true;
        rc522::result_of_card res;
        while (true) {
            if (needs_to_poll) {
                needs_to_poll = false;
                res = cr.poll();
                if (res != rc522::result_of_card::WAIT)
                    break;
            } else {
                __wfi();
            }
        }
        if (res != rc522::result_of_card::SUCC) continue;
        auto uid_buff = cr.uid();
        auto user = lu.at(uid_converter(uid_buff));
        if (!user.has_value()) continue;
        if (user.value().get().sou == status::ADMIN) {
            gpio_put(admin_led, 1);
            ses.run_session();
            gpio_put(admin_led, 0);
        }
        else if (user.value().get().sou == status::USER) {
            gpio_put(user_led, 1);
            std::printf("Hello, %s!\r\n", user.value().get().Name.c_str());
            sleep_ms(1500);
            gpio_put(user_led, 0);
        }
        else if (user.value().get().sou == status::BLACK_LIST) {
            gpio_put(blocked_led, 1);
            std::printf("GET OUT %s\r\n", user.value().get().Name.c_str());
            sleep_ms(1500);
            gpio_put(blocked_led, 0);
        }
    }
    
}
// std::cout << lu.avialiable() << std::endl;
// assert(lu.insert_user(key, "not_admin", status::USER) == insert_result::duplicate_uid);
// assert(lu.remove_user(key) == false);
// assert(lu.change_status(key, status::BLACK_LIST) == false);
// assert(lu.insert_user(77, "too long fuckingly name bruh", status::ADMIN) == insert_result::name_too_long);
// for (uint16_t i{}; i < 1000; i++) {
//     assert(lu.insert_user(i, "luk", status::ADMIN) == insert_result::ok);
//     std::cout << lu.avialiable() << std::endl;
// }
// 