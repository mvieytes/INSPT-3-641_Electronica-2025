#ifndef _DS3231_H_
#define _DS3231_H_
#include "pico/stdlib.h"

/* Dirección I2C */
#define DS3231_I2C_ADDRESS      0x68
/* Dirección del mapa de memoria */
#define DS3231_REG_SECOND       0x00
#define DS3231_REG_MINUTE       0x01
#define DS3231_REG_HOUR         0x02
#define DS3231_REG_DOW          0x03
#define DS3231_REG_DATE         0x04
#define DS3231_REG_MONTH        0x05
#define DS3231_REG_YEAR         0x06
#define DS3231_REG_CONTROL      0x0E
#define DS3231_REG_CTRL_STAT    0x0F

typedef enum DS3231_Rate {
    DS3231_1HZ,
    DS3231_4096HZ,
    DS3231_8192HZ,
    DS3231_32768HZ
} DS3231_Rate;

typedef enum DS3231_SquareWaveEnable {
    DS3231_DISABLED,
    DS3231_ENABLED
} DS3231_SquareWaveEnable;

typedef struct {
    /* data */
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day_of_week;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} ds3231_rtc_t;

/**
 * @brief Get the RTC.
 * @param rtc Binary value of date and time.
 * @retval -1 if error, otherwise number of bytes read
 */
int DS3231_get_rtc(ds3231_rtc_t* rtc);

/**
 * @brief Sets the RTC.
 * @param rtc Binary value of date and time.
 * @retval -1 if error, otherwise number of bytes written
 */
int DS3231_set_rtc(ds3231_rtc_t* rtc);

/**
 * @brief Gets clock halt bit.
 * @param halt halt bit, 0 or 1.
 * @retval -1 if error, otherwise number of bytes read
 */
int DS3231_get_clock_halt(uint8_t* halt);

/**
 * @brief Sets clock halt bit.
 * @param halt Clock halt bit to set, 0 or 1. 0 to start timing, 0 to stop.
 * @retval -1 if error, otherwise number of bytes written
 */
int DS3231_set_clock_halt(uint8_t halt);

/**
 * @brief Initializes the DS3231 module. Sets clock halt bit to 0 to start timing.
 * @brief Initialize RTC whit Monday 01/01/25 00:00:00 - 24hs mode
 * @retval -1 if error, otherwise number of bytes written
 */
int DS3231_rtc_reset(void);

/**
 * @brief Toggle square wave output on pin 7.
 * @param mode DS3231_ENABLED (1) or DS3231_DISABLED (0);
 * @retval -1 if error, otherwise number of bytes written
 */
int DS3231_enable_square_wave(DS3231_SquareWaveEnable mode);

/**
 * @brief Set square wave output frequency.
 * @param rate DS3231_1HZ (0b00), DS3231_4096HZ (0b01), DS3231_8192HZ (0b10) or DS3231_32768HZ (0b11).
 * @retval -1 if error, otherwise number of bytes written
 */
int DS3231_set_interrupt_rate(DS3231_Rate rate);

#endif