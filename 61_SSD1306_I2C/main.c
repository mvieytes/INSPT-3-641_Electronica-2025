#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware.h"
#include "raspberry26x32.h"
#include "ssd1306_i2c.h"

ssd1306_i2c_t ssd1306; // Estructura que define la configuración del HW del OLED

render_area_t frame_area = {
    start_col: 0,
    end_col : SSD1306_WIDTH - 1,
    start_page : 0,
    end_page : SSD1306_NUM_PAGES - 1
};

int main() {
    stdio_init_all();

    ssd1306.i2c_inst = I2C_INST; // Completa los datos en la estructura de configuración
    ssd1306.sda_gpio = I2C_SDA_GPIO;
    ssd1306.scl_gpio = I2C_SCL_GPIO;
    ssd1306.i2c_baudrate = I2C_BAUDRATE;
    ssd1306.i2c_address = SDD1306_ADDRESS;

    SSD1306_init((ssd1306_i2c_t*)(&ssd1306)); // Invoca a la inicialización pasando el puntero a dicha estructura

    calc_render_area_buflen(&frame_area);

    // zero the entire display
    uint8_t buf[SSD1306_BUF_LEN];
    memset(buf, 0, SSD1306_BUF_LEN);
    render(buf, &frame_area);

    // intro sequence: flash the screen 3 times
    for (int i = 0; i < 3; i++) {
        SSD1306_send_cmd(SSD1306_SET_ALL_ON); // Set all pixels on
        sleep_ms(500);
        SSD1306_send_cmd(SSD1306_SET_ENTIRE_ON); // go back to following RAM for pixel state
        sleep_ms(500);
    }

    // render 3 cute little raspberries
    render_area_t area = {
        start_page: 0,
        end_page : (IMG_HEIGHT / SSD1306_PAGE_HEIGHT) - 1
    };

    while (true) {
        area.start_col = 0;
        area.end_col = IMG_WIDTH - 1;

        calc_render_area_buflen(&area);

        uint8_t offset = 5 + IMG_WIDTH; // 5px padding

        for (int i = 0; i < 3; i++) {
            render(raspberry26x32, &area);
            area.start_col += offset;
            area.end_col += offset;
        }

        SSD1306_scroll(true);
        sleep_ms(5000);
        SSD1306_scroll(false);

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
            WriteString(buf, 5, y, text[i]);
            y += 8;
        }
        render(buf, &frame_area);

        // Test the display invert function
        sleep_ms(3000);
        SSD1306_send_cmd(SSD1306_SET_INV_DISP);
        sleep_ms(3000);
        SSD1306_send_cmd(SSD1306_SET_NORM_DISP);

        bool pix = true;
        for (int i = 0; i < 2; i++) {
            for (int x = 0; x < SSD1306_WIDTH; x++) {
                DrawLine(buf, x, 0, SSD1306_WIDTH - 1 - x, SSD1306_HEIGHT - 1, pix);
                render(buf, &frame_area);
            }

            for (int y = SSD1306_HEIGHT - 1; y >= 0; y--) {
                DrawLine(buf, 0, y, SSD1306_WIDTH - 1, SSD1306_HEIGHT - 1 - y, pix);
                render(buf, &frame_area);
            }
            pix = false;
        }
    }
}
