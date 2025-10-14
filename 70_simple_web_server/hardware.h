#ifndef _HARDWARE_H_
#define _HARDWARE_H_
#include "pico/stdlib.h"

#define LED_OFF             (0) 
#define LED_ON              (1)

#define PULS1_PIN           (12)
#define PULS2_PIN           (15)
#define LEDR_PIN            (17)
#define LEDV_PIN            (16)

#define LEDR_OFF()          gpio_put(LEDR_PIN,LED_OFF)
#define LEDR_ON()           gpio_put(LEDR_PIN,LED_ON)
#define LEDR_STATE()        gpio_get(LEDR_PIN)

#define LEDV_OFF()          gpio_put(LEDV_PIN,LED_OFF)
#define LEDV_ON()           gpio_put(LEDV_PIN,LED_ON)
#define LEDV_STATE()        gpio_get(LEDV_PIN)

#define PULS1_STATE()       gpio_get(PULS1_PIN)
#define PULS2_STATE()       gpio_get(PULS2_PIN)

#endif