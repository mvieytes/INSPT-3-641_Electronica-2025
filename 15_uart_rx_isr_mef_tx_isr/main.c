#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware.h"

#include "ring_buffer_tx_uart.h"
#include "mef_recepcion.h"

volatile bool tx_busy = false;
volatile bool rx_saludo = false;
volatile uint8_t tx_byte = 0;

/* Prototipos de funciones */
void isr_uart1(void);

int main() {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Deshabilita FIFO de UART para evaluar apenas se recibe un caracter
    uart_set_fifo_enabled(UART_ID, false);

    // Configurar ISR unificada para RX y TX
    irq_set_exclusive_handler(UART_IRQ_ID, isr_uart1);
    irq_set_enabled(UART_IRQ_ID, true);

    // Habilitar interrupciones RX, TX se habilita cuando hay datos para enviar
    uart_set_irq_enables(UART_ID, true, false);

    while (true) {

        if (rx_saludo == true) {
            // Se recibió el saludo "HOLA"
            rx_saludo = false;
            gpio_put(LED_PIN, !(gpio_get(LED_PIN)));
            ring_buffer_tx_uart_puts("Conmuta LED\n");
        }
        if ((ring_buffer_tx_uart_is_empty() == false) && (tx_busy == false)) {
            //Hay bytes para Tx y la interrupción está apagada
            tx_busy = true;
            // Con FIFO deshabilitado, enviar primer carácter manualmente para arrancar
            if (uart_is_writable(UART_ID)) {
                if (ring_buffer_tx_uart_getc((uint8_t*)(&tx_byte)) == true) {
                    uart_get_hw(UART_ID)->dr = tx_byte;
                }
            }
            uart_set_irq_enables(UART_ID, true, true);  // Habilitar TX interrupt
        }
    }
}

// ISR unificada para RX y TX
void isr_uart1(void) {
    // Manejar interrupción de RX
    if (uart_get_hw(UART_ID)->mis & UART_UARTMIS_RXMIS_BITS) {
        while (uart_is_readable(UART_ID)) {
            //Llama a la MEF que evalúa la recepción
            rx_saludo = mef_recepcion(uart_getc(UART_ID));
        }
    }

    // Manejar interrupción de TX
    if (uart_get_hw(UART_ID)->mis & UART_UARTMIS_TXMIS_BITS) {
        // Con FIFO deshabilitado, siempre hay espacio después de TX interrupt
        if (ring_buffer_tx_uart_getc((uint8_t*)(&tx_byte)) == true) {
            // Enviar el carácter directamente al registro
            uart_get_hw(UART_ID)->dr = tx_byte;
        } else {
            // Buffer vacío, deshabilitar TX interrupt
            uart_set_irq_enables(UART_ID, true, false);
            tx_busy = false;
        }
    }
}
