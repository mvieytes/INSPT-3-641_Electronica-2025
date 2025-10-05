#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware.h"
#include "hardware/adc.h"

#define CANT_MEDICIONES     (10)
#define LAPSO_MEDICION      (100)
#define VCC_POWER_VOLT      (3.3)
#define CUENTAS_12_BITS     (4096)

uint32_t lapso;
uint32_t cont_med, acum_med, prom_med;
float vin_med;

int main() {
    stdio_init_all();

    adc_init();
    adc_gpio_init(ADC_0_GPIO);
    adc_select_input(ADC_CHANNEL_0);

    lapso = to_ms_since_boot(get_absolute_time());

    while (true) {

        if (lapso <= to_ms_since_boot(get_absolute_time())) {
            lapso = lapso + LAPSO_MEDICION;
            acum_med = acum_med + adc_read();
            cont_med++;
            if (cont_med >= CANT_MEDICIONES) {
                prom_med = acum_med / cont_med;
                vin_med = (float)((prom_med * VCC_POWER_VOLT) / CUENTAS_12_BITS);
                printf("Vin: %0.2fV - Cuentas: %4d\n", vin_med, prom_med);
                acum_med = 0;
                cont_med = 0;
            }
        }
    }
}
