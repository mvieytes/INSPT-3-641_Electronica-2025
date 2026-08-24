#ifndef _RING_BUFFER_TX_UARTH_
#define _RING_BUFFER_TX_UARTH_

bool ring_buffer_tx_uart_getc(uint8_t* c);
bool ring_buffer_tx_uart_putc(uint8_t c);
bool ring_buffer_tx_uart_puts(const char* str);
bool ring_buffer_tx_uart_is_full(void);
bool ring_buffer_tx_uart_is_empty(void);

#endif