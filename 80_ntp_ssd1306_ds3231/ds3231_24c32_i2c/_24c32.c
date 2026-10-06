#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ds3231_24c32_i2c.h"

/**
 * @brief Read byte/s starting in the designated pos.
 * @param pos Start address to read.
 * @param len Quantity of bytes to to read.
 * @param dest Buffer to write the readed bytes.
 * @retval -1 if error, otherwise number of bytes read
 */
int _24LC32_read(uint16_t pos, uint16_t len, uint8_t* dest) {
	int rta;
	uint8_t buf[2];

	buf[0] = (uint8_t)(pos / 256);
	buf[1] = (uint8_t)(pos % 256);

	absolute_time_t timeout = make_timeout_time_ms(10);
	rta = i2c_write_blocking_until(local_rtc_mem_i2c->i2c_inst, _24LC32_I2C_ADDRESS, buf, 2, true, timeout);
	if (rta > 0) {
		rta = i2c_read_blocking_until(local_rtc_mem_i2c->i2c_inst, _24LC32_I2C_ADDRESS, dest, len, false, timeout);
	}
	return rta;
}

/**
 * @brief Write byte/s starting in the designated pos.
 * @param pos Start address to write.
 * @param len Quantity of bytes to to write.
 * @param orig Buffer to read the bytes to write.
 * @retval -1 if error, otherwise number of bytes read
 */
int _24LC32_write(uint16_t pos, uint16_t len, uint8_t* orig) {
	int rta;
	uint8_t buf[MEM_LEN_PAGE + 2], i;

	buf[0] = (uint8_t)(pos / 256);
	buf[1] = (uint8_t)(pos % 256);
	for (i = 2;i < (len + 2);i++)
		buf[i] = *orig++;
	absolute_time_t timeout = make_timeout_time_ms(10);
	rta = i2c_write_blocking_until(local_rtc_mem_i2c->i2c_inst, _24LC32_I2C_ADDRESS, buf, (len + 2), false, timeout);
	return rta;
}

/**
 * @brief Read byte/s starting in the designated pos.
 * @param pos Start address to read.
 * @param len Quantity of bytes to to read.
 * @param dest Buffer to write the readed bytes.
 * @retval -1 if error, otherwise number of byte read
 */
int _24LC32_read_buffer(uint16_t pos, uint16_t len, uint8_t* dest) {
	int rta;
	uint8_t buff[MEM_LEN_PAGE], i = 0, j = 0, fin = false;
	uint16_t cont_bytes = 0;

	while (fin == false) {
		fin = true;
		do {
			cont_bytes++;
			pos++;
			i++;
		} while ((cont_bytes < len) && ((pos % MEM_LEN_PAGE) != 0));
		rta = _24LC32_read((pos - i), i, buff);
		if (rta > 0) {
			for (j = 0;j < i;j++)
				*(dest + (cont_bytes - i) + j) = buff[j];
			if (cont_bytes < len) {
				i = 0;
				fin = false;
			}
		} else
			break;
	}
	return rta;
}

/**
 * @brief Write byte/s starting in the designated pos.
 * @param pos Start address to write.
 * @param len Quantity of bytes to to write.
 * @param orig Buffer of bytes to write.
 *	@retval -1 if error, otherwise number of byte written
 */
int _24LC32_write_buffer(uint16_t pos, uint16_t len, uint8_t* orig) {
	int rta;
	uint8_t buff_aux[MEM_LEN_PAGE], i = 0, fin = false;
	uint16_t cont_bytes = 0;

	while (fin == false) {
		fin = true;
		do {
			buff_aux[i++] = *(orig + cont_bytes);
			cont_bytes++;
			pos++;
		} while ((cont_bytes < len) && ((pos % MEM_LEN_PAGE) != 0));
		rta = _24LC32_write((pos - i), i, buff_aux);
		if (rta > 0) {
			if (cont_bytes < len) {
				i = 0;
				fin = false;
			}
		} else
			break;
	}
	return rta;
}
