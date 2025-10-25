#ifndef _HARDWARE_H_
#define _HARDWARE_H_

/* Puerto I2C (i2c0 o i2c1) */
#define I2C_INST i2c0                   //Posibles opciones, i2c0 o i2c1
/* GPIOs usados para I2C */
#define I2C_SDA_GPIO 4                  //Ver el pinout para las distintas posibilidades
#define I2C_SCL_GPIO 5
/* Velocidad I2C (en Hz) */
#define I2C_BAUDRATE 100000             //En Hz, funciona a 100KHz (100000)

#endif