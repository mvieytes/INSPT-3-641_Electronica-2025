#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include"hardware.h"
#include "ds3231_24c32_i2c.h"

#define LEN_CODIGO		(4)			//Cant. de bytes a guardar y leer e memoria

ds3231_24c32_i2c_t ds3231_24c32;    //Estructura que define la configuración del HW del conjunto RTC-MEM
ds3231_rtc_t time_actual;			//Estructura donde está la fecha y hora
uint8_t codigo[LEN_CODIGO + 1];		//LEN_CODIGO bytes en memoria EEPROM + NULL al final
const uint8_t codigo_defecto[] = { '1','2','3','4' };

int main() {
	uint8_t i;

	stdio_init_all();

	// Inicializa los gpio's e instancia de I2C que se utilizará
	gpio_set_function(I2C_SDA_GPIO, GPIO_FUNC_I2C);
	gpio_set_function(I2C_SCL_GPIO, GPIO_FUNC_I2C);

	i2c_init(I2C_INST, I2C_BAUDRATE);

	ds3231_24c32.i2c_inst_init = true;			// Indica que el I2C ya está inicializado
	ds3231_24c32.i2c_inst = I2C_INST;			// Completa los datos en la estructura de configuración
	ds3231_24c32.sda_gpio = I2C_SDA_GPIO;
	ds3231_24c32.scl_gpio = I2C_SCL_GPIO;
	ds3231_24c32.i2c_baudrate = I2C_BAUDRATE;

	DS3231_24C32_init((ds3231_24c32_i2c_t*)(&ds3231_24c32));  // Invoca a la inicialización pasando el puntero a dicha estructura

	/* Inicialia RTC */
	if (DS3231_rtc_reset() < 0) {	// Pone los registros de fecha y hora en 0
		printf("Error reset RTC\n");
	}
	time_actual.seconds = 0;
	time_actual.minutes = 0;
	time_actual.hours = 0;
	time_actual.day = 25;
	time_actual.month = 10;
	time_actual.year = 25;
	if (DS3231_set_rtc(&time_actual) < 0) {
		printf("Error set RTC\n");
	}

	/* Inicializa código */
	if (_24LC32_read_buffer(0, LEN_CODIGO, codigo) < 0) {
		printf("Error leyendo memoria\n");
	}
	codigo[LEN_CODIGO] = 0;	//NULL para cerrar string
	printf("Codigo leido: %s\n", codigo);

	for (i = 0; i < LEN_CODIGO; i++) {
		if ((codigo[i] < '0') || (codigo[i] > '9'))
			break;
	}
	if (i < LEN_CODIGO) {
		for (i = 0; i < LEN_CODIGO; i++)
			codigo[i] = codigo_defecto[i];
		if (_24LC32_write_buffer(0, LEN_CODIGO, codigo) < 0) {
			printf("Error escribiendo memoria\n");
		}
		printf("Codigo escrito (si no hubo error): %s\n", codigo);
	}

	while (true) {
		if (DS3231_get_rtc(&time_actual) < 0) {
			printf("Error leyendo RTC\n");
		}

		printf("%02d:%02d:%02d    %02d/%02d/%02d\n",
			time_actual.hours,
			time_actual.minutes,
			time_actual.seconds,
			time_actual.day,
			time_actual.month,
			time_actual.year);

		sleep_ms(1000);
	}
}
