#ifndef _SSD1306_I2C_H_
#define _SSD1306_I2C_H_

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306_graphs.h"

#define SSD1306_HEIGHT (32)
#define SSD1306_WIDTH (128)

#define SSD1306_PAGE_HEIGHT (8)
#define SSD1306_NUM_PAGES (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN (SSD1306_NUM_PAGES * SSD1306_WIDTH)

#define SSD1306_SET_SEG_REMAP (0xA0)
#define SSD1306_SET_ENTIRE_ON (0xA4)
#define SSD1306_SET_ALL_ON (0xA5)
#define SSD1306_SET_NORM_DISP (0xA6)
#define SSD1306_SET_INV_DISP (0xA7)
#define SSD1306_SET_MUX_RATIO (0xA8)
#define SSD1306_SET_DISP (0xAE)
#define SSD1306_SET_COM_OUT_DIR (0xC0)
#define SSD1306_SET_COM_OUT_DIR_FLIP (0xC0)

#define SSD1306_SET_MEM_MODE (0x20)
#define SSD1306_SET_COL_ADDR (0x21)
#define SSD1306_SET_PAGE_ADDR (0x22)
#define SSD1306_SET_HORIZ_SCROLL (0x26)
#define SSD1306_SET_SCROLL (0x2E)

#define SSD1306_SET_DISP_START_LINE (0x40)

#define SSD1306_SET_CONTRAST (0x81)
#define SSD1306_SET_CHARGE_PUMP (0x8D)

#define SSD1306_SET_DISP_OFFSET (0xD3)
#define SSD1306_SET_DISP_CLK_DIV (0xD5)
#define SSD1306_SET_PRECHARGE (0xD9)
#define SSD1306_SET_COM_PIN_CFG (0xDA)
#define SSD1306_SET_VCOM_DESEL (0xDB)

#define SSD1306_WRITE_MODE (0xFE)
#define SSD1306_READ_MODE (0xFF)

typedef struct
{
    i2c_inst_t *i2c_inst;
    uint sda_gpio;
    uint scl_gpio;
    uint32_t i2c_baudrate;
    uint8_t i2c_address;
    int (*lcd_write)(i2c_inst_t *i2c, uint8_t addr, const uint8_t *src, size_t len, bool nostop);
    void (*sleep)(uint32_t ms);
} ssd1306_i2c_t;

void SSD1306_init(ssd1306_i2c_t *ssd1306_i2c);
void SSD1306_send_cmd(uint8_t cmd);
void SSD1306_send_cmd_list(uint8_t *buf, int num);
void SSD1306_send_buf(uint8_t buf[], int buflen);

#endif