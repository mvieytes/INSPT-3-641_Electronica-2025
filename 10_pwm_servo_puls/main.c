#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware.h"
#include "pico/time.h"
#include "hardware/pwm.h"

volatile uint32_t actual_time;

uint16_t pwm_duty = DUTY_0_GRADOS;
uint slice_num, channel_num;
int16_t sentido = STEP_FORWARD;

void puls_callback(uint gpio, uint32_t event_mask);

int main() {
    stdio_init_all();

    /* Inicialización del pin de entrada pulsador */
    gpio_set_function(PULS_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(PULS_PIN, GPIO_IN);
    gpio_pull_up(PULS_PIN); /* Conecta la reistencia de pull down interna (redundante) */
    gpio_set_input_hysteresis_enabled(PULS_PIN, true); /* habilita la histéresis que ayuda discriminar mejor el rebote */
    /* Aquí se configura el pin fuente de interrupción, el evento (flanco descendente) y la función callback */
    gpio_set_irq_enabled_with_callback(PULS_PIN, GPIO_IRQ_EDGE_FALL, true, puls_callback);
    /* Inicialización del pin salida PWM */
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    channel_num = pwm_gpio_to_channel(SERVO_PIN);

    pwm_set_clkdiv_int_frac(slice_num, PWM_DIV_INT, PWM_DIV_FRAC);
    pwm_set_wrap(slice_num, PWM_WRAP);
    pwm_set_enabled(slice_num, true);
    pwm_set_chan_level(slice_num, channel_num, pwm_duty);

    while (true) {
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
    }
}

/* Callback de atención de interrupción */
void puls_callback(uint gpio, uint32_t event_mask) {
    if ((gpio == PULS_PIN) && (event_mask == GPIO_IRQ_EDGE_FALL)) {
        actual_time = to_ms_since_boot(get_absolute_time()) + DEMORA;
    }
}
