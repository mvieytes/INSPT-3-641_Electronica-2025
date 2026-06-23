#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware.h"
#include "hardware/adc.h"

/* Se puede dejar ambas comentadas p/12bits, o elegir entre 10 u 8 bits */
//#define ADC_8BITS
//#define ADC_10BITS

/* Si está instalada la extensión Teleplot, para ver las gráficas de la salida */
#define USA_TELEPLOT

/* Factor de Oversampling */
#define OSF         (16)
/* Longitud de la media móvil sobre salidas decimadas */
#define AVG_LEN     (8)

uint16_t avg_buffer[AVG_LEN];
int avg_idx = 0;

// Oversampling + decimación: devuelve un valor promedio de OSF lecturas
uint16_t oversample_block() {
    uint32_t sum = 0;
    for (int i = 0; i < OSF; i++) {
        sum += adc_read();
    }
    return sum / OSF;
}

// Media móvil sobre las salidas decimadas
uint16_t moving_average(uint16_t new_sample) {
    avg_buffer[avg_idx] = new_sample;
    avg_idx = (avg_idx + 1) % AVG_LEN;

    uint32_t sum = 0;
    for (int i = 0; i < AVG_LEN; i++) {
        sum += avg_buffer[i];
    }
    return sum / AVG_LEN;
}

int main() {
    stdio_init_all();
    adc_init();
    adc_gpio_init(ADC_0_GPIO);
    adc_select_input(ADC_CHANNEL_0);

    while (1) {
        // Paso 0: medición cruda
        uint16_t counts = adc_read();

        // Paso 1: oversampling + decimación
        uint16_t decimated = oversample_block();

        // Paso 2: media móvil sobre la salida decimada
        uint16_t filtered = moving_average(decimated);

#ifndef USA_TELEPLOT
        printf("Decimado: %u - Filtrado: %u\n", decimated, );
#endif
#ifdef USA_TELEPLOT
        printf(">Crudo:%u\n", counts);
        printf(">Decimado:%u\n", decimated);
        printf(">Filtrado:%u\n", filtered);
#endif
        sleep_ms(1); // ajustá según la tasa deseada
    }
}
