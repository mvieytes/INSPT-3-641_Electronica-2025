#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

#define ADC_CHANNEL 0
#define ADC_PIN 26
#define ADC_BITS 12
#define ADC_HISTOGRAM_SIZE (1u << ADC_BITS)
#define ADC_BLOCK_SAMPLES 10000
#define ADC_NUM_BLOCKS 100
#define ADC_TOTAL_SAMPLES (ADC_BLOCK_SAMPLES * ADC_NUM_BLOCKS)
#define ADC_WARMUP_SAMPLES 64
#define SAMPLE_RATE 200000

static uint16_t sample_buffer[ADC_BLOCK_SAMPLES];
static uint16_t warmup_buffer[ADC_WARMUP_SAMPLES];
static uint32_t histogram[ADC_HISTOGRAM_SIZE];

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

static void accumulate_histogram(const uint16_t *buffer, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        histogram[buffer[i] & (ADC_HISTOGRAM_SIZE - 1u)]++;
    }
}

int main()
{
    stdio_init_all();

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_CHANNEL);
    adc_fifo_setup(true, true, 1, false, false);

    // Configurar frecuencia de muestreo.
    // El periodo entre conversiones es (1 + clkdiv) ciclos del reloj ADC,
    // por lo que para 50 kS/s con clk_adc=48 MHz se necesita clkdiv=959.
    adc_set_clkdiv((48000000.0f / SAMPLE_RATE) - 1.0f);

    int dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC);

    printf("# RP2350 ADC noise capture\r\n");
    printf(
        "# channel=%d gpio=%d sample_rate=%d block_samples=%d blocks=%d total_samples=%d bins=%d\r\n",
        ADC_CHANNEL,
        ADC_PIN,
        SAMPLE_RATE,
        ADC_BLOCK_SAMPLES,
        ADC_NUM_BLOCKS,
        ADC_TOTAL_SAMPLES,
        ADC_HISTOGRAM_SIZE);

    capture_samples(dma_chan, &cfg, warmup_buffer, ADC_WARMUP_SAMPLES);
    for (size_t block = 0; block < ADC_NUM_BLOCKS; block++)
    {
        capture_samples(dma_chan, &cfg, sample_buffer, ADC_BLOCK_SAMPLES);
        accumulate_histogram(sample_buffer, ADC_BLOCK_SAMPLES);
    }

    printf("BEGIN %d\r\n", ADC_HISTOGRAM_SIZE);
    for (size_t code = 0; code < ADC_HISTOGRAM_SIZE; code++)
    {
        printf("%u %lu\r\n", (unsigned int)code, (unsigned long)histogram[code]);
    }
    printf("END\r\n");

    while (true)
    {
        tight_loop_contents();
    }
}
