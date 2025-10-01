#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware.h"

int main() {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    gpio_init(PULS_PIN);
    gpio_set_dir(PULS_PIN, GPIO_IN);
    gpio_pull_up(PULS_PIN);
    gpio_set_input_hysteresis_enabled(PULS_PIN, true);

    while (true) {
        if (gpio_get(PULS_PIN) == false) {
            gpio_put(LED_PIN, 1);
        } else {
            gpio_put(LED_PIN, 0);
        }
    }
}
