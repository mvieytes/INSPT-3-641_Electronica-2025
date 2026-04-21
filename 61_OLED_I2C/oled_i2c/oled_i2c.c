#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "oled_i2c.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

oled_i2c_t me;

void OLED_send_cmd(uint8_t cmd) {
    uint8_t buf[2] = { 0x80, cmd };
    me.lcd_write(me.i2c_inst, (me.i2c_address & OLED_WRITE_MODE), buf, 2, false);
}

void OLED_send_cmd_list(uint8_t* buf, int num) {
    for (int i = 0; i < num; i++)
        OLED_send_cmd(buf[i]);
}

void OLED_send_buf(uint8_t buf[], int buflen) {
    uint8_t* temp_buf = malloc(buflen + 1);

    if (temp_buf == NULL)
        return;

    temp_buf[0] = 0x40;
    memcpy(temp_buf + 1, buf, buflen);

    me.lcd_write(me.i2c_inst, (me.i2c_address & OLED_WRITE_MODE), temp_buf, buflen + 1, false);

    free(temp_buf);
}

void OLED_init(oled_i2c_t* oled_i2c) {

    me = (oled_i2c_t)(*oled_i2c);

    // En power-on el OLED suele tardar mas que la Pico en quedar listo.
    sleep_ms(OLED_POWER_ON_DELAY_MS);

    i2c_init(me.i2c_inst, me.i2c_baudrate);
    gpio_set_function(me.sda_gpio, GPIO_FUNC_I2C);
    gpio_set_function(me.scl_gpio, GPIO_FUNC_I2C);
    gpio_pull_up(me.sda_gpio);
    gpio_pull_up(me.scl_gpio);

    me.lcd_write = i2c_write_blocking;
    me.sleep = sleep_ms;

    me.sleep(10);

    uint8_t cmds[] = {
    OLED_CMD_DISPLAY,
#if (OLED_CONTROLLER == OLED_CTRL_SSD1306)
    OLED_CMD_MEMORY_MODE,
    0x00,
#endif
    OLED_CMD_DISPLAY_START_LINE,
    OLED_CMD_SEG_REMAP | 0x01,
    OLED_CMD_MUX_RATIO,
    OLED_HEIGHT - 1,
    OLED_CMD_COM_OUT_DIR | 0x08,
    OLED_CMD_DISPLAY_OFFSET,
    0x00,
    OLED_CMD_COM_PIN_CFG,
#if (OLED_HEIGHT == 32)
    0x02,
#elif (OLED_HEIGHT == 64)
    0x12,
#else
    0x02,
#endif
    OLED_CMD_DISPLAY_CLK_DIV,
    0x80,
    OLED_CMD_PRECHARGE,
#if (OLED_CONTROLLER == OLED_CTRL_SH1106)
    0x1F,
#else
    0xF1,
#endif
    OLED_CMD_VCOM_DESEL,
    0x30,
    OLED_CMD_CONTRAST,
    0xFF,
    OLED_CMD_ENTIRE_ON,
    OLED_CMD_NORMAL_DISPLAY,
#if (OLED_CONTROLLER == OLED_CTRL_SH1106)
    SH1106_CMD_DC_DC_CTRL,
    0x8B,
#else
    OLED_CMD_CHARGE_PUMP,
    0x14,
    OLED_CMD_SCROLL | 0x00,
#endif
    OLED_CMD_DISPLAY | 0x01,
    };

    OLED_send_cmd_list(cmds, count_of(cmds));
    me.sleep(OLED_POST_INIT_DELAY_MS);
}
