#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "pico/time.h"
#include "hardware/pwm.h"
#include "hardware.h"

#define ENCENDIENDO     (1)
#define APAGANDO        (-1)

volatile uint32_t actual_time;
int8_t sentido = ENCENDIENDO;

uint16_t pwm_duty = PWM_MIN_DUTY;
float div;
uint slice_num, channel_num;

int main() {
    stdio_init_all();

    uint32_t freq_hz = clock_get_hz(clk_sys);
    div = (float)(freq_hz / ((PWM_WRAP + 1.0) * (PWM_FREC_HZ)));

    /* Inicializaci�n del pin salida PWM */
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    channel_num = pwm_gpio_to_channel(SERVO_PIN);

    pwm_set_clkdiv(slice_num, div);
    pwm_set_wrap(slice_num, PWM_WRAP);
    pwm_set_enabled(slice_num, true);
    pwm_set_chan_level(slice_num, channel_num, pwm_duty);

    actual_time = to_ms_since_boot(get_absolute_time());

    while (true) {
        if (actual_time < to_ms_since_boot(get_absolute_time())) {
            actual_time += PWM_LAP_TIME;
            if (sentido == ENCENDIENDO) {
                if (pwm_duty < PWM_MAX_DUTY) {
                    pwm_duty += PWM_STEP_DUTY;
                } else {
                    sentido = APAGANDO;
                    pwm_duty -= PWM_STEP_DUTY;
                }
            } else {
                if (pwm_duty >= (PWM_MIN_DUTY + PWM_STEP_DUTY)) {
                    pwm_duty -= PWM_STEP_DUTY;
                } else {
                    sentido = ENCENDIENDO;
                    pwm_duty += PWM_STEP_DUTY;
                }
            }
            pwm_set_chan_level(slice_num, channel_num, pwm_duty);
        }
    }
}
