#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware.h"

#include "lcd_i2c.h"

/* Estructura que define la configuración del HW del LCD */
lcd_i2c_t lcd;

uint8_t contador, contador2;
uint32_t lapso;

int main() {

    stdio_init_all();

    /* Estas líneas de código son necesarias para inicializar el HW del LCD */
    /* -------------------------------------------------------------------- */
    /* Inicializan el HW del uC para el LCD */
    /* Completa los datos en la estructura de configuración */
    lcd.i2c_inst = I2C_INST;
    lcd.sda_gpio = I2C_SDA_GPIO;
    lcd.scl_gpio = I2C_SCL_GPIO;
    lcd.i2c_baudrate = I2C_BAUDRATE;
    lcd.i2c_address = LCD_ADDRESS;
    lcd.lcd_lines = LCD_LINE_QTY;
    /* Invoca a la inicialización pasando el puntero a dicha estructura */
    lcd_i2c_init((lcd_i2c_t*)(&lcd));
    /* -------------------------------------------------------------------- */
    /* Hasta aquí */

    /* Borra y prepara LCD para comenar a escribir */
    lcd_clear();
    lcd_return_home();
    /* Línea y posición cursor */
    lcd_set_cursor(LCD_1ST_LINE, 0);
    /* Escribe un string */
    lcd_print("HOLA MUNDO");
    if (lcd.lcd_lines == LCD_LINE_QTY_4) {
        lcd_set_cursor(LCD_3RD_LINE, 5);
        lcd_print("TERCER LINEA");
    }

    lapso = to_ms_since_boot(get_absolute_time());

    while (1) {
        if (lapso <= to_ms_since_boot(get_absolute_time())) {
            lapso += 1000;
            lcd_set_cursor(LCD_2ND_LINE, 0);
            /* Escribe un caracter ASCII */
            lcd_print_char((contador / 100) | 0x30);
            lcd_print_char(((contador % 100) / 10) | 0x30);
            lcd_print_char((contador % 10) | 0x30);
            lcd_set_cursor(LCD_4TH_LINE, 17);
            if (lcd.lcd_lines == LCD_LINE_QTY_4) {
                contador2 = 100 - contador;
                lcd_print_char((contador2 / 100) | 0x30);
                lcd_print_char(((contador2 % 100) / 10) | 0x30);
                lcd_print_char((contador2 % 10) | 0x30);
            }
            contador++;
        }
    }
}
