#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

#define ADC_NUM_SAMPLES 4000 // 80 ciclos exactos a 1kHz con fs=50kHz (coherent sampling)
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

    // Configurar frecuencia de muestreo.
    // El periodo entre conversiones es (1 + clkdiv) ciclos del reloj ADC,
    // por lo que para 50 kS/s con clk_adc=48 MHz se necesita clkdiv=959.
    adc_set_clkdiv((48000000.0f / SAMPLE_RATE) - 1.0f);

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
