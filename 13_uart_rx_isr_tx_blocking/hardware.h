#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#define LED_PIN         (22)

// Configuración de UART1 (id de uart, irq de uart y baudrate)
#define UART_ID         (uart1)
#define UART_IRQ_ID     (UART1_IRQ)
#define BAUD_RATE       (115200)

// Pines asociados a UART1 en la Pico
#define UART_TX_PIN     (4)
#define UART_RX_PIN     (5)

#endif /* _HARDWARE_H_ */