#ifndef _DS3231_24C32_I2C_H_
#define _DS3231_24C32_I2C_H_

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ds3231.h"
#include "_24c32.h"

typedef struct {
    bool i2c_inst_init;
    i2c_inst_t* i2c_inst;
    uint sda_gpio;
    uint scl_gpio;
    uint32_t i2c_baudrate;
} ds3231_24c32_i2c_t;
extern ds3231_24c32_i2c_t* local_rtc_mem_i2c;

void DS3231_24C32_init(ds3231_24c32_i2c_t* config);

#endif // _DS3231_24C32_I2C_H_