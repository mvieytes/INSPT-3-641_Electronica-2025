#ifndef _SSD1306_GRAPHICS_H_
#define _SSD1306_GRAPHICS_H_

typedef struct {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;
    int32_t buflen;
}render_area_t;

void SSD1306_calc_render_area_buflen(render_area_t* area);
void SSD1306_render(uint8_t* buf, render_area_t* area);
void SSD1306_scroll(bool on);
void SSD1306_write_string(uint8_t* buf, int16_t x, int16_t y, char* str);
void SSD1306_draw_line(uint8_t* buf, int x0, int y0, int x1, int y1, bool on);

#endif