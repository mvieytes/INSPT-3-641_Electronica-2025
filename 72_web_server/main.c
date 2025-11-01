#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware.h"
#include "wifi_sta.h"
#include "web_server.h"
#include "html.h"

/* Reemplazar aquí con el SSID y PASS de nuestro AP */
const char my_ssid[] = "MI_SSID";
const char my_pass[] = "MI_PASS";

wifi_sta_data_t* wifi_sta_actual;
uint8_t mi_ip[4];

web_server_data_t* web_server_actual;

bool soc_cyw43_init(void);

int main() {
    wifi_sta_err_t error;
    bool soc_cyw43_ok = false;

    stdio_init_all();

    // Config GPIO salida (LED)
    gpio_set_function(LED_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    // Config GPIO entrada (PULS)
    gpio_set_function(PULS_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(PULS_PIN, GPIO_IN);
    gpio_pull_up(PULS_PIN);
    gpio_set_input_hysteresis_enabled(PULS_PIN, true);

    if (soc_cyw43_init()) {
        soc_cyw43_ok = true;
        printf("MAIN: SoC CYW43 inicializado!!!\r\n");
    } else {
        soc_cyw43_ok = false;
        printf("MAIN: Error al inicializar SoC CYW43\r\n");
    }

    set_ssid_to_connect(my_ssid);
    set_pass_to_connect(my_pass);
    set_html_to_serve(html_page);
    set_html_actions(html_do_actions);

    wifi_sta_actual = ptr_wifi_data();
    web_server_actual = ptr_web_server_data();

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
                }
            }
            web_server_fsm((uint8_t)(wifi_sta_actual->wifi_status & WIFI_CONECTED));
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
