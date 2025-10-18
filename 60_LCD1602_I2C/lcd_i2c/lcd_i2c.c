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

lcd_i2c_t me;

void lcd_i2c_put_4bits(uint8_t value);
void lcd_i2c_write_byte(uint8_t value, uint8_t mode);

void lcd_print_char(uint8_t dato) {
    lcd_i2c_write_byte(dato, LCD_DATA);
}

void lcd_print(uint8_t* buff) {
    while (*buff) {
        lcd_print_char(*buff++);
    }
}

void lcd_clear(void) {
    lcd_i2c_write_byte(LCD_CLEARDISPLAY, LCD_COMMAND);
    me.sleep(2);
}

void lcd_return_home(void) {
    lcd_i2c_write_byte(LCD_RETURNHOME, LCD_COMMAND);
    me.sleep(2);
}

void lcd_set_cursor(line_nro_t row, uint8_t col) {
    int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
    lcd_i2c_write_byte((LCD_SETDDRAMADDR | (col + row_offsets[row])), LCD_COMMAND);
}

void lcd_set_back_on(void) {
    me.stat_backl |= LCD_BACKL_MASK;
    me.lcd_write(me.i2c_inst, me.i2c_address, &me.stat_backl, 1, 0);
}

void lcd_set_back_off(void) {
    me.stat_backl &= ~LCD_BACKL_MASK;
    me.lcd_write(me.i2c_inst, me.i2c_address, &me.stat_backl, 1, 0);
}

/* Write byte (command or data) */
void lcd_i2c_write_byte(uint8_t value, uint8_t mode) {
    uint8_t high, low;

    high = (uint8_t)(value & 0xF0);
    low = (uint8_t)(value << 4);
    lcd_i2c_put_4bits(high | (me.stat_backl & LCD_BACKL_MASK) | (mode & 0x07));
    lcd_i2c_put_4bits(low | (me.stat_backl & LCD_BACKL_MASK) | (mode & 0x07));
    me.sleep(1);
}
/* Put one nibble plus enable */
void lcd_i2c_put_4bits(uint8_t value) {
    me.lcd_write(me.i2c_inst, me.i2c_address, &value, 1, 0);
    value |= LCD_ENABLE_MASK;
    me.lcd_write(me.i2c_inst, me.i2c_address, &value, 1, 0);
    value &= ~LCD_ENABLE_MASK;
    me.lcd_write(me.i2c_inst, me.i2c_address, &value, 1, 0);
}

void lcd_i2c_init(lcd_i2c_t* lcd_i2c) {
    uint8_t aux;

    me = (lcd_i2c_t)(*lcd_i2c);
    me.stat_backl = LCD_BACKL_MASK;

    me.lcd_write = i2c_write_blocking;
    me.sleep = sleep_ms;

    gpio_set_function(me.sda_gpio, GPIO_FUNC_I2C);
    gpio_set_function(me.scl_gpio, GPIO_FUNC_I2C);

    i2c_init(me.i2c_inst, me.i2c_baudrate);

    me.sleep(50);
    lcd_i2c_put_4bits(0x30 | (me.stat_backl & LCD_BACKL_MASK) | (LCD_COMMAND & 0x07));
    me.sleep(5);
    lcd_i2c_put_4bits(0x30 | (me.stat_backl & LCD_BACKL_MASK) | (LCD_COMMAND & 0x07));
    me.sleep(1);
    lcd_i2c_put_4bits(0x30 | (me.stat_backl & LCD_BACKL_MASK) | (LCD_COMMAND & 0x07));
    me.sleep(5);
    lcd_i2c_put_4bits(0x20 | (me.stat_backl & LCD_BACKL_MASK) | (LCD_COMMAND & 0x07));
    me.sleep(2);

    aux = 0 | (LCD_FUNCTIONSET | LCD_4BITMODE | LCD_5x8DOTS);
    if (me.lcd_lines > LCD_LINE_QTY_1)
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
}
