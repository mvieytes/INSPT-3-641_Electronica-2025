#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware.h"

volatile uint32_t demora1, demora2;

/*
Prototipo de la función callback de gpio
Esta función recibe 2 parámetros (gpio y evento) y no devuelve ningún resultado
Se pone este prototipo para que se pueda registrar el callback, debajo de "main" está el código
*/
void puls_callback(uint gpio, uint32_t event_mask);

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

    /*
    Aquí se configura:
    Pin (GPIO) fuente de interrupción
    Evento que dispara la interrupción (el flanco descendente)
    Y se registra la función callback (puls_callback)
    */
    gpio_set_irq_enabled_with_callback(PULS1_PIN, GPIO_IRQ_EDGE_FALL, true, puls_callback);
    /*
    Aquí se configura:
    Pin (GPIO) fuente de interrupción
    Evento que dispara la interrupción (el flanco descendente)
    El callback es el mismo registrado previamente, los GPIO usan UN ÚNICO callabck y se registra una vez
    */
    gpio_set_irq_enabled(PULS2_PIN, GPIO_IRQ_EDGE_FALL, true);

    while (true) {
        if ((demora1 != 0) && (demora1 < to_ms_since_boot(get_absolute_time()))) {
            /*
            Si había un valor en demora (porque hubo un flanco descendente)
            Y se alcanzó ese valor (instante del flanco descendente + 30mS)
            Se pone "demora" en '0' para no volver a entrar aquí
            Y si el pulsador sigue presionado se cambia el estado del LED
            */
            demora1 = 0;
            if (gpio_get(PULS1_PIN) == 0) {
                /* Le indico al programa que está habilitado para hacer lo que deba hacer */
                gpio_put(LEDR_PIN, !(gpio_get(LEDR_PIN)));
            }
        }
        if ((demora2 != 0) && (demora2 < to_ms_since_boot(get_absolute_time()))) {
            /*
            Todo igual para el otro pulsador y LED.
            */
            demora2 = 0;
            if (gpio_get(PULS2_PIN) == 0) {
                /* Le indico al programa que está habilitado para hacer lo que deba hacer */
                gpio_put(LEDV_PIN, !(gpio_get(LEDV_PIN)));
            }
        }
    }
}

void puls_callback(uint gpio, uint32_t event_mask) {
    /* Ahora llama al callback si se produce un flanco descendente */
    if ((gpio == PULS1_PIN) && (event_mask == GPIO_IRQ_EDGE_FALL)) {
        if (demora1 == 0)
            demora1 = to_ms_since_boot(get_absolute_time()) + DEMORA;
        /*
        "get_absolute_time" devuelve la cantidad de tics del oscilador desde que se encendió
        "to_ms_since_boot" pasa a mS esa cantidad
        A todo eso le sumo 30mS para esperar que pasen los posibles rebotes del pulsador
        */
    }
    if ((gpio == PULS2_PIN) && (event_mask == GPIO_IRQ_EDGE_FALL)) {
        if (demora2 == 0)
            demora2 = to_ms_since_boot(get_absolute_time()) + DEMORA;
        /*
        Todo igual para el segundo pulsador
        */
    }
}
