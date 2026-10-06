#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"
#include "hardware.h"
#include "ds3231_24c32_i2c.h"
#include "ssd1306_i2c.h"
#include "wifi_sta.h"
#include "ntp.h"

#include "pantalla.h"

/* Reemplazar aquí con el SSID y PASS de nuestro AP */
const char my_ssid[] = "my_SSID";
const char my_pass[] = "my_PASS";

ds3231_24c32_i2c_t ds3231_24c32;    //Estructura que define la configuración del HW del conjunto RTC-MEM
ds3231_rtc_t time_actual;			//Estructura donde está la fecha y hora

ssd1306_i2c_t ssd1306_i2c;          //Estructura que define la configuración del HW del OLED

wifi_sta_data_t* wifi_sta_actual;
uint8_t mi_ip[4];
ntp_result_t* ntp_actual;

bool soc_cyw43_init(void);

int main() {
    wifi_sta_err_t error;
    bool soc_cyw43_ok = false;
    uint8_t buff_aux[24];

    stdio_init_all();

    // Config GPIO salida (LED)
    gpio_set_function(LED_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    // Config GPIO entrada (PULS)
    gpio_set_function(PULS_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(PULS_PIN, GPIO_IN);
    gpio_pull_up(PULS_PIN);
    gpio_set_input_hysteresis_enabled(PULS_PIN, true);

    gpio_set_function(I2C_SDA_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_GPIO, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_GPIO);
    gpio_pull_up(I2C_SCL_GPIO);

    // Inicializa los gpio's e instancia de I2C que se utilizará
    i2c_init(I2C_INST, I2C_BAUDRATE);

    ds3231_24c32.i2c_inst_init = true;			// Indica que el I2C ya está inicializado
    ds3231_24c32.i2c_inst = I2C_INST;			// Completa los datos en la estructura de configuración
    ds3231_24c32.sda_gpio = I2C_SDA_GPIO;
    ds3231_24c32.scl_gpio = I2C_SCL_GPIO;
    ds3231_24c32.i2c_baudrate = I2C_BAUDRATE;

    DS3231_24C32_init((ds3231_24c32_i2c_t*)(&ds3231_24c32));  // Invoca a la inicialización pasando el puntero a dicha estructura

    ssd1306_i2c.i2c_inst_init = true;			// Indica que el I2C ya está inicializado
    ssd1306_i2c.i2c_inst = I2C_INST;            // Completa los datos en la estructura de configuración
    ssd1306_i2c.sda_gpio = I2C_SDA_GPIO;
    ssd1306_i2c.scl_gpio = I2C_SCL_GPIO;
    ssd1306_i2c.i2c_baudrate = I2C_BAUDRATE;

    SSD1306_init((ssd1306_i2c_t*)(&ssd1306_i2c)); // Invoca a la inicialización pasando el puntero a dicha estructura

    LimpiaDisplay();
    DibujaLogoChico();

    if (soc_cyw43_init()) {
        soc_cyw43_ok = true;
        printf("MAIN: SoC CYW43 inicializado!!!\r\n");
    } else {
        soc_cyw43_ok = false;
        printf("MAIN: Error al inicializar SoC CYW43\r\n");
    }

    set_ssid_to_connect(my_ssid);
    set_pass_to_connect(my_pass);

    wifi_sta_actual = ptr_wifi_data();
    ntp_actual = ptr_ntp_actual();

    while (true) {
        if (soc_cyw43_ok) {
            wifi_sta_fsm();
            if ((uint8_t)(wifi_sta_actual->wifi_status & WIFI_CONECTED)) {
                if (mi_ip[0] != wifi_sta_actual->my_ip[0]) {
                    mi_ip[0] = wifi_sta_actual->my_ip[0];
                    mi_ip[1] = wifi_sta_actual->my_ip[1];
                    mi_ip[2] = wifi_sta_actual->my_ip[2];
                    mi_ip[3] = wifi_sta_actual->my_ip[3];
                    printf("MAIN: IP address %d.%d.%d.%d\r\n", mi_ip[0], mi_ip[1], mi_ip[2], mi_ip[3]);

                    EscribeRenglon("CONECTADO", 0, 36);
                    sprintf(buff_aux, "%d %d %d %d", mi_ip[0], mi_ip[1], mi_ip[2], mi_ip[3]);
                    EscribeRenglon(buff_aux, 1, 36);
                }
            }
            ntp_fsm((uint8_t)(wifi_sta_actual->wifi_status & WIFI_CONECTED));
            if (ntp_actual->new_time) {
                ntp_actual->new_time = false;
                printf("MAIN: Fecha y hora NTP: %02d/%02d/%04d %02d:%02d:%02d\n",
                    ntp_actual->ajutime->tm_mday, ntp_actual->ajutime->tm_mon + 1,
                    ntp_actual->ajutime->tm_year + 1900, ntp_actual->ajutime->tm_hour,
                    ntp_actual->ajutime->tm_min, ntp_actual->ajutime->tm_sec);

                sprintf(buff_aux, "%d %d %d",
                    ntp_actual->ajutime->tm_hour,
                    ntp_actual->ajutime->tm_min,
                    ntp_actual->ajutime->tm_sec);
                EscribeRenglon(buff_aux, 2, 36);
                sprintf(buff_aux, "%d %d %d",
                    ntp_actual->ajutime->tm_mday,
                    ntp_actual->ajutime->tm_mon + 1,
                    (ntp_actual->ajutime->tm_year - 100));
                EscribeRenglon(buff_aux, 3, 36);
            }
        }
    }
}

/*
    Inicializa el SoC CYW43 de la placa W o 2W
    Devuelve:
    - CYW43_INIT_OK si no hubo errores
    - CYW43_INIT_FAIL si hay error
*/
bool soc_cyw43_init(void) {

    if (cyw43_arch_init() == 0) {
        return true;
    }
    return false;
}
