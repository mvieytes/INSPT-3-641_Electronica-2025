#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware.h"

bool flag = false;
volatile uint32_t demora;

int main() {
    stdio_init_all();

    gpio_init_mask((1 << LEDV_PIN) | (1 << LEDR_PIN) | (1 << PULS1_PIN) | (1 << PULS2_PIN));
    gpio_set_dir_out_masked((1 << LEDV_PIN) | (1 << LEDR_PIN));
    gpio_put_masked(((1 << LEDV_PIN) | (1 << LEDR_PIN)), 0);
    gpio_set_dir_in_masked((1 << PULS1_PIN) | (1 << PULS2_PIN));

    gpio_pull_up(PULS1_PIN);
    gpio_pull_up(PULS2_PIN);

    gpio_set_input_hysteresis_enabled(PULS1_PIN, true);
    gpio_set_input_hysteresis_enabled(PULS2_PIN, true);

    while (true) {
        if ((gpio_get(PULS1_PIN) == 0) && (flag == false)) {
            /*
            Cuando se produce un flanco descendente se pone "flag"" en '1' para evitar volver a entrar aquí
            "get_absolute_time" devuelve la cantidad de tics del oscilador desde que se encendió
            "to_ms_since_boot" pasa a mS esa cantidad
            A eso se suma DEMORA (30mS)
            Se guarda en "demora" si no estaba inicializada previamente
            */
            flag = true;
            if (demora == 0)
                demora = to_ms_since_boot(get_absolute_time()) + DEMORA;
        }
        if ((gpio_get(PULS1_PIN) == 1) && (flag == true)) {
            /* Si se libera el pulsador, se reinicia "flag" para normalizar condiciones iniciales */
            flag = false;
        }

        if ((demora != 0) && (demora < to_ms_since_boot(get_absolute_time()))) {
            /*
            Si había un valor en demora (porque hubo un flanco descendente)
            Y se alcanzó ese valor (instante del flanco descendente + 30mS)
            Se pone "demora" en '0' para no volver a entrar aquí
            Y si el pulsador sigue presionado se cambia el estado del LED
            */
            demora = 0;
            if (gpio_get(PULS1_PIN) == 0) {
                gpio_put(LEDR_PIN, !(gpio_get(LEDR_PIN)));
            }
        }
    }
}
