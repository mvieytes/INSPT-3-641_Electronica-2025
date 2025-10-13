#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware.h"
#include "pico/time.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

volatile uint32_t actual_time;

volatile uint32_t adc_lap;
uint32_t cont_med, acum_med, prom_med, resu_med;

uint16_t pwm_duty = DUTY_0_GRADOS;
uint slice_num, channel_num;
int16_t sentido = STEP_FORWARD;

void puls_callback(uint gpio, uint32_t event_mask);

int main() {
    stdio_init_all();

    /* Inicialización del pin de entrada pulsador */
#ifdef USA_PULSADOR
    gpio_set_function(PULS_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(PULS_PIN, GPIO_IN);
    gpio_pull_up(PULS_PIN); /* Conecta la reistencia de pull down interna (redundante) */
    gpio_set_input_hysteresis_enabled(PULS_PIN, true); /* habilita la histéresis que ayuda discriminar mejor el rebote */
    /* Aquí se configura el pin fuente de interrupción, el evento (flanco descendente) y la función callback */
    gpio_set_irq_enabled_with_callback(PULS_PIN, GPIO_IRQ_EDGE_FALL, true, puls_callback);
#endif
#ifdef USA_ADC
    /* Aquí se configura el ADC */
    adc_init();
    adc_gpio_init(ADC_0_GPIO);
    adc_select_input(ADC_CHANNEL_0);
#endif
    /* Inicialización del pin salida PWM */
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    channel_num = pwm_gpio_to_channel(SERVO_PIN);
    adc_lap = to_ms_since_boot(get_absolute_time());

    pwm_set_clkdiv_int_frac(slice_num, PWM_DIV_INT, PWM_DIV_FRAC);
    pwm_set_wrap(slice_num, PWM_WRAP);
    pwm_set_enabled(slice_num, true);
    pwm_set_chan_level(slice_num, channel_num, pwm_duty);

    while (true) {
#ifdef USA_PULSADOR
        if ((actual_time) && (actual_time < to_ms_since_boot(get_absolute_time()))) {
            actual_time = 0;
            if (gpio_get(PULS_PIN) == 0) {
                if (pwm_duty >= DUTY_180_GRADOS) {
                    pwm_duty = DUTY_180_GRADOS;
                    sentido = STEP_BACKWARD;
                } else if (pwm_duty <= DUTY_0_GRADOS) {
                    pwm_duty = DUTY_0_GRADOS;
                    sentido = STEP_FORWARD;
                }
                pwm_duty += sentido;
                pwm_set_chan_level(slice_num, channel_num, pwm_duty);
            }
        }
#endif
#ifdef USA_ADC
        if (adc_lap <= to_ms_since_boot(get_absolute_time())) {
            adc_lap += 20;
            resu_med = adc_read();
            acum_med = acum_med + resu_med;
            cont_med++;
            if (cont_med >= CANT_MEDICIONES) {
                /* Cada 200mS */
                prom_med = acum_med / cont_med;
                /* prom_med puede variar entre 0 y 4095 y se necesita entre 500 y 2500 */
                /* debemos mapear... */
                pwm_duty = (((prom_med / 2) + 500) * 9812) / 10000;    //entre 500 y 2500,09
                pwm_set_chan_level(slice_num, channel_num, pwm_duty);
                acum_med = 0;
                cont_med = 0;
            }
        }
#endif
    }
}

/* Callback de atención de interrupción */
void puls_callback(uint gpio, uint32_t event_mask) {
    if ((gpio == PULS_PIN) && (event_mask == GPIO_IRQ_EDGE_FALL)) {
        actual_time = to_ms_since_boot(get_absolute_time()) + DEMORA;
    }
}
