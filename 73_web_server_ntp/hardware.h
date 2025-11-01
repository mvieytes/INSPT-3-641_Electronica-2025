#ifndef _HARDWARE_H_
#define _HARDWARE_H_
#include "pico/stdlib.h"

#define OFF                 (0) 
#define ON                  (1)

#define PULS_PIN            (15)
#define LED_PIN             (22)

#define LED_OFF()           gpio_put(LED_PIN,OFF)
#define LED_ON()            gpio_put(LED_PIN,ON)
#define LED_STATE()         gpio_get(LED_PIN)

#define PULS_STATE()        gpio_get(PULS_PIN)

#endif