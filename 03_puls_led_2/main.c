#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware.h"

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
        if (gpio_get(PULS1_PIN) == 0) {
            gpio_put(LEDR_PIN, 1);
        }
        if (gpio_get(PULS2_PIN) == 0) {
            gpio_put(LEDR_PIN, 0);
        }
    }
}
