#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#define RING_BUFFER_TX_UART_SIZE 256

volatile uint8_t ring_buffer_tx_uart[RING_BUFFER_TX_UART_SIZE];
volatile uint16_t ring_buffer_tx_uart_head = 0;
volatile uint16_t ring_buffer_tx_uart_tail = 0;

/*
 * Verifica buffer, retorna true si está completo
 */
bool ring_buffer_tx_uart_is_full(void) {
    return ((ring_buffer_tx_uart_head + 1) % RING_BUFFER_TX_UART_SIZE) == ring_buffer_tx_uart_tail;
}

/*
 * Verifica buffer, retorna true si está vacío
 */
bool ring_buffer_tx_uart_is_empty(void) {
    return ring_buffer_tx_uart_head == ring_buffer_tx_uart_tail;
}

/*
 * Si hay un byte en el buffer lo pasa y retorna true
 * Si está vacío, retorna false
 */
bool ring_buffer_tx_uart_getc(uint8_t* c) {
    bool rta = false;

    if (ring_buffer_tx_uart_is_empty() == false) {
        // Hay al menos un caracter, lo lee para sacarlo
        *c = ring_buffer_tx_uart[ring_buffer_tx_uart_tail];
        ring_buffer_tx_uart_tail = (ring_buffer_tx_uart_tail + 1) % RING_BUFFER_TX_UART_SIZE;
        rta = true;
    }
    return rta;
}

/*
 * Si hay lugar en el buffer, guarda el byte y retorna true
 * Si está completo, retorna false
 */
bool ring_buffer_tx_uart_putc(uint8_t c) {
    bool rta = false;

    if (ring_buffer_tx_uart_is_full() == false) {
        // Hay espacio, agregar el carácter al buffer
        ring_buffer_tx_uart[ring_buffer_tx_uart_head] = c;
        ring_buffer_tx_uart_head = (ring_buffer_tx_uart_head + 1) % RING_BUFFER_TX_UART_SIZE;
        rta = true;
    }
    return rta;
}

/*
 * Mientras haya lugar en el buffer va almacenando los bytes
 * Si termina de almacenar devuelve true
 * Si no tiene o se queda sin espacio, devuelve false
 */
bool ring_buffer_tx_uart_puts(const char* str) {
    bool rta = true;

    while (*str) {
        if (ring_buffer_tx_uart_putc(*str) == false) {
            rta = false;
            break;
        }
        str++;
    }
    return rta;
}
