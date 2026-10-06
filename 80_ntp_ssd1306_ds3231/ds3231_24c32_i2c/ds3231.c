#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ds3231_24c32_i2c.h"

const unsigned char days_of_months[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
const unsigned short months_offset[] = { 0,31,59,90,120,151,181,212,243,273,304,334 };

/**
 * @brief Decodes the raw binary value stored in registers to decimal format.
 * @param bin Binary-coded decimal value retrieved from register, 0 to 255.
 * @return Decoded decimal value.
 */
uint8_t DS3231_decode_BCD(uint8_t bin) {
    return (((bin & 0xf0) >> 4) * 10) + (bin & 0x0f);
}

/**
 * @brief Encodes a decimal number to binaty-coded decimal for storage in registers.
 * @param dec Decimal number to encode.
 * @return Encoded binary-coded decimal value.
 */
uint8_t DS3231_encode_BCD(uint8_t dec) {
    return (((dec / 10) << 4) | (dec % 10));
}

/**
 * @brief Sets the byte in the designated DS3231 register to value.
 * @param reg Register address to write.
 * @param value Value to set, 0 to 255.
 */
int DS3231_set_reg(uint8_t reg, uint8_t value) {
    int rta;
    uint8_t buf[2] = { reg, value };

    absolute_time_t timeout = make_timeout_time_ms(2);
    rta = i2c_write_blocking_until(local_rtc_mem_i2c->i2c_inst, DS3231_I2C_ADDRESS, buf, 2, true, timeout);
    return rta;
}

/**
 * @brief Gets the byte in the designated DS3231 register.
 * @param reg Register address to read.
 * @param *value Value stored in the register, 0 to 255.
 * @retval -1 if error, otherwise number of bytes read
 */
int DS3231_get_reg(uint8_t reg, uint8_t* value) {
    int rta;

    absolute_time_t timeout = make_timeout_time_ms(2);
    rta = i2c_write_blocking_until(local_rtc_mem_i2c->i2c_inst, DS3231_I2C_ADDRESS, &reg, 1, true, timeout);
    if (rta > 0) {
        rta = i2c_read_blocking_until(local_rtc_mem_i2c->i2c_inst, DS3231_I2C_ADDRESS, value, 1, false, timeout);
    }
    return rta;
}

/**
 * @brief Return TRUE i it's leap year.
 * @param year 4 digit year.
 * @return True or False.
 */
bool YearIsLeap(unsigned short year) {
    if ((((year % 4) == 0) && ((year % 100) != 0)) || ((year % 400) == 0)) return true;
    return false;
}

/**
 * @brief Complete with the Day of Week starting 1-Sunday, 2-Monday, etc.
 * @param rtc Struct with actual date.
 * @return Day of Week value 1 to 7.
 */
uint8_t GregorianDay(ds3231_rtc_t* rtc) {
    uint32_t last_year, year = 2000;
    uint32_t leaps_to_date;
    uint32_t dow;

    year += rtc->year;
    last_year = year - 1;

    leaps_to_date = ((last_year / 4) - (last_year / 100) + (last_year / 400));

    if (YearIsLeap(year))    dow = 1;
    else                    dow = 0;

    dow += (last_year * 365) + leaps_to_date + months_offset[rtc->month - 1] + rtc->day;

    rtc->day_of_week = (uint8_t)((dow % 7) + 1);
    return (uint8_t)((dow % 7) + 1);
}

/**
 * @brief Sets clock halt bit.
 * @param halt Clock halt bit to set, 0 or 1. 0 to start timing, 0 to stop.
 * @retval -1 if error, otherwise number of bytes written
 */
int DS3231_set_clock_halt(uint8_t halt) {
    int rta;
    uint8_t value;
    uint8_t ch = (halt ? 1 << 7 : 0);

    rta = DS3231_get_reg((DS3231_REG_CONTROL), &value);
    if (rta > 0) {
        rta = DS3231_set_reg(DS3231_REG_CONTROL, (ch | (value & 0x7f)));
    }
    return rta;
}

/**
 * @brief Gets clock halt bit.
 * @param halt halt bit, 0 or 1.
 * @retval -1 if error, otherwise number of bytes read
 */
int DS3231_get_clock_halt(uint8_t* halt) {
    int rta;

    rta = DS3231_get_reg(DS3231_REG_CONTROL, halt);
    *halt >>= 7;
    return rta;
}

/**
 * @brief Get the RTC.
 * @param rtc Binary value of date and time.
 * @retval -1 if error, otherwise number of bytes read
 */
int DS3231_get_rtc(ds3231_rtc_t* rtc) {
    int rta;
    uint8_t buf[8], i;

    buf[0] = DS3231_REG_SECOND;

    absolute_time_t timeout = make_timeout_time_ms(2);
    rta = i2c_write_blocking_until(local_rtc_mem_i2c->i2c_inst, DS3231_I2C_ADDRESS, (const uint8_t*)(buf), 1, true, timeout);
    if (rta > 0) {
        rta = i2c_read_blocking_until(local_rtc_mem_i2c->i2c_inst, DS3231_I2C_ADDRESS, buf, 8, false, timeout);
        if (rta > 0) {
            rtc->seconds = DS3231_decode_BCD(buf[0]);
            rtc->minutes = DS3231_decode_BCD(buf[1]);
            rtc->hours = DS3231_decode_BCD(buf[2]);
            rtc->day_of_week = DS3231_decode_BCD(buf[3]);
            rtc->day = DS3231_decode_BCD(buf[4]);
            rtc->month = DS3231_decode_BCD(buf[5]);
            rtc->year = DS3231_decode_BCD(buf[6]);
        }
    }
    return rta;
}

/**
 * @brief Sets the RTC.
 * @param rtc Binary value of date and time.
 * @retval -1 if error, otherwise number of bytes written
 */
int DS3231_set_rtc(ds3231_rtc_t* rtc) {
    int rta;
    uint8_t buf[8], i;

    (void)GregorianDay(rtc);
    buf[0] = DS3231_REG_SECOND;
    buf[1] = DS3231_encode_BCD(rtc->seconds);
    buf[2] = DS3231_encode_BCD(rtc->minutes);
    buf[3] = DS3231_encode_BCD(rtc->hours);
    buf[4] = DS3231_encode_BCD(rtc->day_of_week);
    buf[5] = DS3231_encode_BCD(rtc->day);
    buf[6] = DS3231_encode_BCD(rtc->month);
    buf[7] = DS3231_encode_BCD(rtc->year);

    absolute_time_t timeout = make_timeout_time_ms(2);

    DS3231_set_clock_halt(0);
    rta = i2c_write_blocking_until(local_rtc_mem_i2c->i2c_inst, DS3231_I2C_ADDRESS, (const uint8_t*)(buf), 8, false, timeout);
    DS3231_set_clock_halt(1);
    return rta;
}

/**
 * @brief Initializes the DS3231 module. Sets clock halt bit to 0 to start timing.
 * @brief Initialize RTC whit Monday 01/01/25 00:00:00 - 24hs mode
 * @retval -1 if error, otherwise number of bytes written
 */
int DS3231_rtc_reset(void) {
    ds3231_rtc_t rtc_aux;

    rtc_aux = (ds3231_rtc_t){ 0x00,0x00,0x00,0x00,0x01,0x01,0x25 };
    return (DS3231_set_rtc(&rtc_aux));
}

/**
 * @brief Toggle square wave output on pin 7.
 * @param mode DS3231_ENABLED (1) or DS3231_DISABLED (0);
 * @retval -1 if error, otherwise number of bytes written
 */
int DS3231_enable_square_wave(DS3231_SquareWaveEnable mode) {
    int rta;
    uint8_t controlReg;
    uint8_t newControlReg;

    rta = DS3231_get_reg(DS3231_REG_CONTROL, &controlReg);
    if (rta > 0) {
        newControlReg = (controlReg & ~(1 << 4)) | ((mode & 1) << 4);
        rta = DS3231_set_reg(DS3231_REG_CONTROL, newControlReg);
    }
    return rta;
}

/**
 * @brief Set square wave output frequency.
 * @param rate DS3231_1HZ (0b00), DS3231_4096HZ (0b01), DS3231_8192HZ (0b10) or DS3231_32768HZ (0b11).
 * @retval -1 if error, otherwise number of bytes written
 */
int DS3231_set_interrupt_rate(DS3231_Rate rate) {
    int rta;
    uint8_t controlReg;
    uint8_t newControlReg;

    rta = DS3231_get_reg(DS3231_REG_CONTROL, &controlReg);
    if (rta > 0) {
        newControlReg = (controlReg & ~0x03) | rate;
        rta = DS3231_set_reg(DS3231_REG_CONTROL, newControlReg);
    }
    return rta;
}
