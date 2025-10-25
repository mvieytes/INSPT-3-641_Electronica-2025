#ifndef _24LC32_H_
#define _24LC32_H_
#include "pico/stdlib.h"

/* Dirección I2C */
#define _24LC32_I2C_ADDRESS 0x57

/* Descripción del mapa de memoria */
#define MEM_BOTTOM_ADDRESS	0
#define MEM_TOP_ADDRESS		4096
#define MEM_LEN_PAGE		32

/**
 * @brief Read byte/s starting in the designated pos.
 * @param pos Start address to read.
 * @param len Quantity of bytes to to read.
 * @param dest Buffer to write the readed bytes.
 * @retval -1 if error, otherwise number of byte read
 */
int _24LC32_read_buffer(uint16_t pos, uint16_t len, uint8_t* dest);

/**
 * @brief Write byte/s starting in the designated pos.
 * @param pos Start address to write.
 * @param len Quantity of bytes to to write.
 * @param orig Buffer of bytes to write.
 *	@retval -1 if error, otherwise number of byte written
 */
int _24LC32_write_buffer(uint16_t pos, uint16_t len, uint8_t* orig);

#endif