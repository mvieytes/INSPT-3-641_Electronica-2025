#include <stdint.h>
#include "lcd_i2c.h"

#define LCD_COMMAND                 (0)
#define LCD_DATA                    (1)

#define LCD_ENABLE_MASK             (1 << 2)
#define LCD_BACKL_MASK              (1 << 3)

// commands
#define LCD_CLEARDISPLAY            (1 << 0)
#define LCD_RETURNHOME              (1 << 1)
#define LCD_ENTRYMODESET            (1 << 2)
#define LCD_DISPLAYCONTROL          (1 << 3)
#define LCD_CURSORSHIFT             (1 << 4)
#define LCD_FUNCTIONSET             (1 << 5)
#define LCD_SETCGRAMADDR            (1 << 6)
#define LCD_SETDDRAMADDR            (1 << 7)

// flags for display entry mode
#define LCD_ENTRYRIGHT              (0x00)
#define LCD_ENTRYLEFT               (0x02)
#define LCD_ENTRYSHIFTDECREMENT     (0x00)
#define LCD_ENTRYSHIFTINCREMENT     (0x01)

// flags for display on/off control
#define LCD_DISPLAYON               (0x04)
#define LCD_DISPLAYOFF              (0x00)
#define LCD_CURSORON                (0x02)
#define LCD_CURSOROFF               (0x00)
#define LCD_BLINKON                 (0x01)
#define LCD_BLINKOFF                (0x00)

// flags for display/cursor shift
#define LCD_DISPLAYMOVE             (0x08)
#define LCD_CURSORMOVE              (0x00)
#define LCD_MOVERIGHT               (0x04)
#define LCD_MOVELEFT                (0x00)

// flags for function set
#define LCD_8BITMODE                (0x10)
#define LCD_4BITMODE                (0x00)
#define LCD_2LINE                   (0x08)
#define LCD_1LINE                   (0x00)
#define LCD_5x10DOTS                (0x04)
#define LCD_5x8DOTS                 (0x00)

lcd_i2c_t* local_lcd_i2c;

int lcd_i2c_put_4bits(uint8_t value);
int lcd_i2c_write_byte(uint8_t value, uint8_t mode);

int lcd_print_char(uint8_t dato) {
    return(lcd_i2c_write_byte(dato, LCD_DATA));
}

int lcd_print(uint8_t* buff) {
    int rta;
    while (*buff) {
        rta = lcd_print_char(*buff++);
        if (rta < 0) break;
    }
    return rta;
}

int lcd_clear(void) {
    int rta;
    rta = lcd_i2c_write_byte(LCD_CLEARDISPLAY, LCD_COMMAND);
    sleep_ms(2);
    return rta;
}

int lcd_return_home(void) {
    int rta;
    rta = lcd_i2c_write_byte(LCD_RETURNHOME, LCD_COMMAND);
    sleep_ms(2);
    return rta;
}

int lcd_set_cursor(line_nro_t row, uint8_t col) {
    int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
    return (lcd_i2c_write_byte((LCD_SETDDRAMADDR | (col + row_offsets[row])), LCD_COMMAND));
}

int lcd_set_back_on(void) {
    uint8_t value;

    value = local_lcd_i2c->stat_backl & (~LCD_BACKL_MASK);
    absolute_time_t timeout = make_timeout_time_ms(2);
    return (i2c_write_blocking_until(local_lcd_i2c->i2c_inst, LCD_ADDRESS, &value, 1, false, timeout) < 0);
}

int lcd_set_back_off(void) {
    uint8_t value;

    value = local_lcd_i2c->stat_backl & (~LCD_BACKL_MASK);
    absolute_time_t timeout = make_timeout_time_ms(2);
    return (i2c_write_blocking_until(local_lcd_i2c->i2c_inst, LCD_ADDRESS, &value, 1, false, timeout) < 0);
}

/* Write byte (command or data) */
int lcd_i2c_write_byte(uint8_t value, uint8_t mode) {
    int rta = -1;
    uint8_t high, low;

    high = (uint8_t)(value & 0xF0);
    low = (uint8_t)(value << 4);
    do {
        if (lcd_i2c_put_4bits(high | (local_lcd_i2c->stat_backl & LCD_BACKL_MASK) | (mode & 0x07)) < 0)
            break;
        if (lcd_i2c_put_4bits(low | (local_lcd_i2c->stat_backl & LCD_BACKL_MASK) | (mode & 0x07)) < 0)
            break;
        rta = 0;
        sleep_ms(1);
    } while (0);
    return rta;
}
/* Put one nibble plus enable */
int lcd_i2c_put_4bits(uint8_t value) {
    int rta = -1;

    absolute_time_t timeout = make_timeout_time_ms(2);
    do {
        if (i2c_write_blocking_until(local_lcd_i2c->i2c_inst, LCD_ADDRESS, &value, 1, false, timeout) < 0)
            break;
        value |= LCD_ENABLE_MASK;
        if (i2c_write_blocking_until(local_lcd_i2c->i2c_inst, LCD_ADDRESS, &value, 1, false, timeout) < 0)
            break;
        value &= ~LCD_ENABLE_MASK;
        if (i2c_write_blocking_until(local_lcd_i2c->i2c_inst, LCD_ADDRESS, &value, 1, false, timeout) < 0)
            break;
        rta = 0;
    } while (0);
    return rta;
}

int lcd_i2c_init(lcd_i2c_t* config) {
    int rta = -1;
    uint8_t aux;

    local_lcd_i2c = config;
    local_lcd_i2c->stat_backl = LCD_BACKL_MASK;

    if (local_lcd_i2c->i2c_inst_init == false) {
        gpio_set_function(local_lcd_i2c->sda_gpio, GPIO_FUNC_I2C);
        gpio_set_function(local_lcd_i2c->scl_gpio, GPIO_FUNC_I2C);

        i2c_init(local_lcd_i2c->i2c_inst, local_lcd_i2c->i2c_baudrate);
        local_lcd_i2c->i2c_inst_init = true;
    }

    do {
        sleep_ms(50);
        if (lcd_i2c_put_4bits(0x30 | (local_lcd_i2c->stat_backl & LCD_BACKL_MASK) | (LCD_COMMAND & 0x07)) < 0)
            break;
        sleep_ms(5);
        if (lcd_i2c_put_4bits(0x30 | (local_lcd_i2c->stat_backl & LCD_BACKL_MASK) | (LCD_COMMAND & 0x07)) < 0)
            break;
        sleep_ms(1);
        if (lcd_i2c_put_4bits(0x30 | (local_lcd_i2c->stat_backl & LCD_BACKL_MASK) | (LCD_COMMAND & 0x07)) < 0)
            break;
        sleep_ms(5);
        if (lcd_i2c_put_4bits(0x20 | (local_lcd_i2c->stat_backl & LCD_BACKL_MASK) | (LCD_COMMAND & 0x07)) < 0)
            break;
        sleep_ms(2);

        aux = 0 | (LCD_FUNCTIONSET | LCD_4BITMODE | LCD_5x8DOTS);
        if (local_lcd_i2c->lcd_lines > LCD_LINE_QTY_1)
            aux |= LCD_2LINE;
        else
            aux |= LCD_1LINE;
        lcd_i2c_write_byte(aux, LCD_COMMAND);

        aux = 0 | (LCD_CURSORSHIFT | LCD_CURSORMOVE | LCD_MOVELEFT);
        lcd_i2c_write_byte(aux, LCD_COMMAND);

        aux = 0 | LCD_CLEARDISPLAY;
        lcd_i2c_write_byte(aux, LCD_COMMAND);

        aux = (LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT);
        lcd_i2c_write_byte(aux, LCD_COMMAND);

        aux = (LCD_DISPLAYCONTROL | LCD_DISPLAYON);
        lcd_i2c_write_byte(aux, LCD_COMMAND);
        lcd_clear();

    } while (0);
    return rta;
}
