#ifndef _HARDWARE_H_
#define _HARDWARE_H_

/* Puerto I2C (i2c0 o i2c1) */
#define I2C_INST        i2c0        //Posibles opciones, i2c0 o i2c1
/* GPIOs usados para I2C */
#define I2C_SDA_GPIO    4          //Ver el pinout para las distintas posibilidades
#define I2C_SCL_GPIO    5
/* Velocidad I2C (en Hz) */
#define I2C_BAUDRATE    400000     //En Hz, puede funcionar a 100KHz (100000) o 400KHz (400000)
/* Cantidad de líneas */
#define LCD_LINE_QTY    2          //Para LCD de 2 líneas, puede ser también 1 o 4 líneas

#endif