#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ds3231_24c32_i2c.h"

ds3231_24c32_i2c_t* local_rtc_mem_i2c;

void DS3231_24C32_init(ds3231_24c32_i2c_t* config) {

    local_rtc_mem_i2c = config;

    if (local_rtc_mem_i2c->i2c_inst == false) {
        gpio_set_function(local_rtc_mem_i2c->sda_gpio, GPIO_FUNC_I2C);
        gpio_set_function(local_rtc_mem_i2c->scl_gpio, GPIO_FUNC_I2C);

        i2c_init(local_rtc_mem_i2c->i2c_inst, local_rtc_mem_i2c->i2c_baudrate);
        local_rtc_mem_i2c->i2c_inst_init = true;
    }
}
