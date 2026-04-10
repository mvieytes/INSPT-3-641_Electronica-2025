/*
Inyectar en el canal 0 del ADC una señal senoidal que abarque desde el 10% al 90%
del rango de entrada (aproximadamente 0.33V a 2.97V) a una frecuencia de 1kHz (Vpp = 2.64V, Voffset = 1.65V).
El programa debe muestrear esta señal a una tasa de al menos 50kS/s utilizando el ADC con DMA,
y enviar los datos muestreados por UART en un formato que permita al host identificar claramente
el inicio y el final de cada bloque de muestras, así como la cantidad de muestras transmitidas.
El programa debe incluir un mecanismo para estabilizar el arranque del ADC antes de comenzar a
capturar el bloque principal de muestras, como una captura dummy corta.
Antes de correr el programa, verificar de que la señal senoidal esté correctamente conectada
al pin GPIO 26 (ADC0) del Raspberry Pi Pico, y que el host esté listo para recibir los datos por UART.
(ejecutar desde la terminal integrada del VSCode lo siguiente: python capture_uart_samples.py COMx,
donde x corresponde al número de puerto COM asignado al dispositivo USB del Pico).
Una vez capturados los datos, ejecutar python test_SINAD_ENOB.py samples.txt para calcular
el SINAD y ENOB de la señal muestreada.

*/
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

#define ADC_NUM_SAMPLES 4096
#define ADC_WARMUP_SAMPLES 32
#define ADC_PIN 26 // GPIO 26 es Canal 0
#define SAMPLE_RATE 50000

uint16_t sample_buffer[ADC_NUM_SAMPLES];
uint16_t warmup_buffer[ADC_WARMUP_SAMPLES];

static void capture_samples(int dma_chan, dma_channel_config *cfg, uint16_t *buffer, size_t count)
{
    dma_channel_configure(dma_chan, cfg,
                          buffer,
                          &adc_hw->fifo,
                          count,
                          true);

    adc_run(true);
    dma_channel_wait_for_finish_blocking(dma_chan);
    adc_run(false);
    adc_fifo_drain();
}

int main()
{
    stdio_init_all();

    // 1. Inicializar ADC
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(0);
    adc_fifo_setup(true, true, 1, false, false); // FIFO activo, DMA activo

    // Configurar frecuencia de muestreo (48MHz / clkdiv = f_s)
    // clkdiv = 48000000 / 50000 = 960. El hardware resta 1 automáticamente.
    adc_set_clkdiv(48000000.0f / SAMPLE_RATE);

    // 2. Configurar DMA
    int dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC); // Sincronizar con el ADC

    while (true)
    {
        // Captura dummy corta para estabilizar el arranque antes del bloque medido.
        capture_samples(dma_chan, &cfg, warmup_buffer, ADC_WARMUP_SAMPLES);
        capture_samples(dma_chan, &cfg, sample_buffer, ADC_NUM_SAMPLES);

        // 3. Enviar datos por UART con framing para que el parser del host pueda resincronizar.
        printf("BEGIN %d\r\n", ADC_NUM_SAMPLES);
        for (int i = 0; i < ADC_NUM_SAMPLES; i++)
        {
            printf("%04u\r\n", sample_buffer[i] & 0x0FFF);
        }
        printf("END\r\n");

        while (1)
        {
            tight_loop_contents(); // Evita que el programa termine
        }
    }
}
