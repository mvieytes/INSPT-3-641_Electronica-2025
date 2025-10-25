#ifndef _LCD_I2C_H_
#define _LCD_I2C_H_

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define LCD_ADDRESS     (0x27)                //Dirección más difundida, verificar si corresponde a nuestro LCD

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
    bool i2c_inst_init;
    i2c_inst_t* i2c_inst;
    uint sda_gpio;
    uint scl_gpio;
    uint32_t i2c_baudrate;
    line_nro_t lcd_lines;
    uint8_t stat_backl;
} lcd_i2c_t;

int lcd_print_char(uint8_t dato);
int lcd_print(uint8_t* buff);
int lcd_clear(void);
int lcd_return_home(void);
int lcd_set_cursor(line_nro_t row, uint8_t col);
int lcd_set_back_on(void);
int lcd_set_back_off(void);
int lcd_i2c_init(lcd_i2c_t* config);

#endif