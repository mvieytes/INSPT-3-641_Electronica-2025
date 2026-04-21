#ifndef _OLED_I2C_H_
#define _OLED_I2C_H_

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "oled_graphics.h"

#define OLED_CTRL_SSD1306 (0)
#define OLED_CTRL_SH1106  (1)

// Seleccionar el controlador a compilar.
#ifndef OLED_CONTROLLER
#define OLED_CONTROLLER OLED_CTRL_SH1106
#endif

// Retardos configurables para mejorar el arranque en frio del display.
#ifndef OLED_POWER_ON_DELAY_MS
#define OLED_POWER_ON_DELAY_MS (200)
#endif

#ifndef OLED_POST_INIT_DELAY_MS
#define OLED_POST_INIT_DELAY_MS (20)
#endif

#define OLED_HEIGHT (64)

#if (OLED_CONTROLLER == OLED_CTRL_SH1106)
#define OLED_WIDTH (128)
#define OLED_RAM_WIDTH (132)
#define OLED_COLUMN_OFFSET (2)
#define OLED_SUPPORTS_HORIZONTAL_ADDRESSING (0)
#elif (OLED_CONTROLLER == OLED_CTRL_SSD1306)
#define OLED_WIDTH (128)
#define OLED_RAM_WIDTH (128)
#define OLED_COLUMN_OFFSET (0)
#define OLED_SUPPORTS_HORIZONTAL_ADDRESSING (1)
#else
#error "OLED_CONTROLLER debe ser OLED_CTRL_SSD1306 u OLED_CTRL_SH1106"
#endif

#define OLED_PAGE_HEIGHT (8)
#define OLED_NUM_PAGES (OLED_HEIGHT / OLED_PAGE_HEIGHT)
#define OLED_BUF_LEN (OLED_NUM_PAGES * OLED_WIDTH)

#define OLED_CMD_SEG_REMAP (0xA0)
#define OLED_CMD_ENTIRE_ON (0xA4)
#define OLED_CMD_ALL_ON (0xA5)
#define OLED_CMD_NORMAL_DISPLAY (0xA6)
#define OLED_CMD_INVERT_DISPLAY (0xA7)
#define OLED_CMD_MUX_RATIO (0xA8)
#define OLED_CMD_DISPLAY (0xAE)
#define OLED_CMD_COM_OUT_DIR (0xC0)
#define OLED_CMD_COM_OUT_DIR_FLIP (0xC0)

#define OLED_CMD_MEMORY_MODE (0x20)
#define OLED_CMD_COLUMN_ADDR (0x21)
#define OLED_CMD_PAGE_ADDR (0x22)
#define OLED_CMD_HORIZONTAL_SCROLL (0x26)
#define OLED_CMD_SCROLL (0x2E)

#define OLED_CMD_DISPLAY_START_LINE (0x40)

#define OLED_CMD_CONTRAST (0x81)
#define OLED_CMD_CHARGE_PUMP (0x8D)

#define OLED_CMD_DISPLAY_OFFSET (0xD3)
#define OLED_CMD_DISPLAY_CLK_DIV (0xD5)
#define OLED_CMD_PRECHARGE (0xD9)
#define OLED_CMD_COM_PIN_CFG (0xDA)
#define OLED_CMD_VCOM_DESEL (0xDB)

#define SH1106_CMD_PAGE_ADDR (0xB0)
#define SH1106_CMD_LOW_COL_ADDR (0x00)
#define SH1106_CMD_HIGH_COL_ADDR (0x10)
#define SH1106_CMD_DC_DC_CTRL (0xAD)

#define OLED_WRITE_MODE (0xFE)
#define OLED_READ_MODE (0xFF)

typedef struct {
    i2c_inst_t* i2c_inst;
    uint sda_gpio;
    uint scl_gpio;
    uint32_t i2c_baudrate;
    uint8_t i2c_address;
    int (*lcd_write)(i2c_inst_t* i2c, uint8_t addr, const uint8_t* src, size_t len, bool nostop);
    void (*sleep)(uint32_t ms);
} oled_i2c_t;

void OLED_init(oled_i2c_t* oled_i2c);
void OLED_send_cmd(uint8_t cmd);
void OLED_send_cmd_list(uint8_t* buf, int num);
void OLED_send_buf(uint8_t buf[], int buflen);

#endif