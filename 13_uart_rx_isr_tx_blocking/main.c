#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware.h"

volatile uint8_t rx_byte;

/* Este es el callback de RX UART */
void isr_rx_uart1(void);

int main() {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    irq_set_exclusive_handler(UART_IRQ_ID, isr_rx_uart1);
    irq_set_enabled(UART_IRQ_ID, true);

    uart_set_irq_enables(UART_ID, true, false);

    while (true) {

        if (rx_byte != 0) {
            uart_putc(UART_ID, rx_byte); // Eco del byte recibido
            uart_putc(UART_ID, '\n');
            if (rx_byte == '1') {
                gpio_put(LED_PIN, 1);
                uart_puts(UART_ID, "LED ON\n");
            }
            if (rx_byte == '2') {
                gpio_put(LED_PIN, 0);
                uart_puts(UART_ID, "LED OFF\n");
            }
            rx_byte = 0;
        }
    }
}

void isr_rx_uart1(void) {
    while (uart_is_readable(UART_ID)) {
        rx_byte = uart_getc(UART_ID);
        printf("rx\n");
    }
}
