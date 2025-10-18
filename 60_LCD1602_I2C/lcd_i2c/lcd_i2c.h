#ifndef _LCD_I2C_H_
#define _LCD_I2C_H_

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

typedef enum {
    LCD_LINE_QTY_1 = 1,
    LCD_LINE_QTY_2,
    LCD_LINE_QTY_3,
    LCD_LINE_QTY_4,
} line_qty_t;

typedef enum {
    LCD_1ST_LINE,
    LCD_2ND_LINE,
    LCD_3RD_LINE,
    LCD_4TH_LINE,
} line_nro_t;

typedef struct {
    i2c_inst_t* i2c_inst;
    uint sda_gpio;
    uint scl_gpio;
    uint32_t i2c_baudrate;
    uint8_t i2c_address;
    line_nro_t lcd_lines;
    uint8_t stat_backl;
    int (*lcd_write)(i2c_inst_t* i2c, uint8_t addr, const uint8_t* src, size_t len, bool nostop);
    void (*sleep)(uint32_t ms);
} lcd_i2c_t;

void lcd_print_char(uint8_t dato);
void lcd_print(uint8_t* buff);
void lcd_clear(void);
void lcd_return_home(void);
void lcd_set_cursor(line_nro_t row, uint8_t col);
void lcd_set_back_on(void);
void lcd_set_back_off(void);
void lcd_i2c_init_hw(void);
void lcd_i2c_init(lcd_i2c_t* lcd_i2c);

#endif