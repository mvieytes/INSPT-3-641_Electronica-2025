#ifndef _OLED_GRAPHICS_H_
#define _OLED_GRAPHICS_H_

typedef struct {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;
    int32_t buflen;
} oled_render_area_t;

void OLED_calc_render_area_buflen(oled_render_area_t* area);
void OLED_render(uint8_t* buf, oled_render_area_t* area);
void OLED_scroll(bool on);
void OLED_write_string(uint8_t* buf, int16_t x, int16_t y, char* str);
void OLED_draw_line(uint8_t* buf, int x0, int y0, int x1, int y1, bool on);

#endif