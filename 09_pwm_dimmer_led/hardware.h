#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#define SERVO_PIN       (17)

#define PWM_FREC_HZ     (5000)
#define PWM_WRAP        (9999)
#define PWM_MIN_DUTY    (0)
#define PWM_MAX_DUTY    (PWM_WRAP+1)

#define PWM_LAP_TIME    (5) //En mS, cada cuanto se incrementa o decrementa el duty
#define PWM_STEP_QTY    (1000 / PWM_LAP_TIME)
#define PWM_STEP_DUTY   (PWM_MAX_DUTY / PWM_STEP_QTY)

#endif