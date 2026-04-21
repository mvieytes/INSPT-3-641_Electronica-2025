#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware.h"
#include "raspberry26x32.h"
#include "oled_i2c.h"

oled_i2c_t oled; // Estructura que define la configuración del HW del OLED

oled_render_area_t frame_area = {
    start_col: 0,
    end_col : OLED_WIDTH - 1,
    start_page : 0,
    end_page : OLED_NUM_PAGES - 1
};

int main() {
    stdio_init_all();

    oled.i2c_inst = I2C_INST; // Completa los datos en la estructura de configuración
    oled.sda_gpio = I2C_SDA_GPIO;
    oled.scl_gpio = I2C_SCL_GPIO;
    oled.i2c_baudrate = I2C_BAUDRATE;
    oled.i2c_address = SDD1306_ADDRESS;

    OLED_init((oled_i2c_t*)(&oled)); // Invoca a la inicialización pasando el puntero a dicha estructura

    OLED_calc_render_area_buflen(&frame_area);

    // zero the entire display
    uint8_t buf[OLED_BUF_LEN];
    memset(buf, 0, OLED_BUF_LEN);
    OLED_render(buf, &frame_area);

    // intro sequence: flash the screen 3 times
    for (int i = 0; i < 3; i++) {
        OLED_send_cmd(OLED_CMD_ALL_ON); // Set all pixels on
        sleep_ms(500);
        OLED_send_cmd(OLED_CMD_ENTIRE_ON); // go back to following RAM for pixel state
        sleep_ms(500);
    }

    // render 3 cute little raspberries
    oled_render_area_t area = {
        start_page: 0,
        end_page : (IMG_HEIGHT / OLED_PAGE_HEIGHT) - 1
    };

    while (true) {
        area.start_col = 0;
        area.end_col = IMG_WIDTH - 1;

        OLED_calc_render_area_buflen(&area);

        uint8_t offset = 5 + IMG_WIDTH; // 5px padding

        for (int i = 0; i < 3; i++) {
            OLED_render(raspberry26x32, &area);
            area.start_col += offset;
            area.end_col += offset;
        }

        OLED_scroll(true);
        sleep_ms(5000);
        OLED_scroll(false);

        char* text[] = {
            "A long time ago",
            "  on an OLED ",
            "   display",
            " far far away",
            "Lived a small",
            "red raspberry",
            "by the name of",
            "    PICO" };

        int y = 0;
        for (int i = 0; i < count_of(text); i++) {
            OLED_write_string(buf, 5, y, text[i]);
            y += 8;
        }
        OLED_render(buf, &frame_area);

        // Test the display invert function
        sleep_ms(3000);
        OLED_send_cmd(OLED_CMD_INVERT_DISPLAY);
        sleep_ms(3000);
        OLED_send_cmd(OLED_CMD_NORMAL_DISPLAY);

        bool pix = true;
        for (int i = 0; i < 2; i++) {
            for (int x = 0; x < OLED_WIDTH; x++) {
                OLED_draw_line(buf, x, 0, OLED_WIDTH - 1 - x, OLED_HEIGHT - 1, pix);
                OLED_render(buf, &frame_area);
            }

            for (int y = OLED_HEIGHT - 1; y >= 0; y--) {
                OLED_draw_line(buf, 0, y, OLED_WIDTH - 1, OLED_HEIGHT - 1 - y, pix);
                OLED_render(buf, &frame_area);
            }
            pix = false;
        }
    }
}
