#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware.h"
#include "simple_ws.h"
#include "html.h"

/* Reemplazar aquí con el SSID y PASS de nuestro AP */
const char my_ssid[] = "Gwynt";
const char my_pass[] = "G44dG3rl!";

error_t soc_cyw43_init(void);

int main() {
    error_t error;
    bool soc_cyw43_ok = false;

    stdio_init_all();

    // Config GPIO salida (LEDs)
    gpio_set_function_masked(((1 << LEDR_PIN) | (1 << LEDV_PIN)), GPIO_FUNC_SIO);
    gpio_set_dir_out_masked((1 << LEDR_PIN) | (1 << LEDV_PIN));
    gpio_put_masked(((1 << LEDR_PIN) | (1 << LEDV_PIN)), 0);
    // Config GPIO entrada (PULs)
    gpio_set_function_masked(((1 << PULS1_PIN) | (1 << PULS2_PIN)), GPIO_FUNC_SIO);
    gpio_set_dir_in_masked((1 << PULS1_PIN) | (1 << PULS2_PIN));
    gpio_pull_up(PULS1_PIN);
    gpio_pull_up(PULS2_PIN);
    gpio_set_input_hysteresis_enabled(PULS1_PIN, true);
    gpio_set_input_hysteresis_enabled(PULS2_PIN, true);

    error = soc_cyw43_init();
    if (error == CYW43_INIT_OK) {
        soc_cyw43_ok = true;
        printf("SoC CYW43 inicializado!!!\r\n");
    } else {
        soc_cyw43_ok = false;
        printf("Error al inicializar SoC CYW43\r\n");
    }

    set_ssid_to_connect(my_ssid);
    set_pass_to_connect(my_pass);
    set_html_to_serve(html_page);
    set_html_actions(html_do_actions);

    web_server_init_data();

    while (true) {
        if (soc_cyw43_ok) {
            web_server_fsm();
        }
    }
}

/*
    Inicializa el SoC CYW43 de la placa W o 2W
    Devuelve:
    - CYW43_INIT_OK si no hubo errores
    - CYW43_INIT_FAIL si hay error
*/
error_t soc_cyw43_init(void) {
    error_t err = CYW43_INIT_OK;

    if (cyw43_arch_init()) {
        err = CYW43_INIT_FAIL;
    }
    return err;
}
