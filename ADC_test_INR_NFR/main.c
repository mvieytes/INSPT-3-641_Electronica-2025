#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

#define ADC_CHANNEL 0
#define ADC_PIN 26
#define ADC_NUM_SAMPLES 8192
#define ADC_WARMUP_SAMPLES 64
#define SAMPLE_RATE 20000
uint16_t sample_buffer[ADC_NUM_SAMPLES];
uint16_t warmup_buffer[ADC_WARMUP_SAMPLES];

static void capture_samples(int dma_chan, dma_channel_config* cfg, uint16_t* buffer, size_t count) {
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

int main() {
    stdio_init_all();

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_CHANNEL);
    adc_fifo_setup(true, true, 1, false, false);

    adc_set_clkdiv(48000000.0f / SAMPLE_RATE);

    int dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC);

    printf("# RP2350 ADC noise capture\r\n");
    printf("# channel=%d gpio=%d sample_rate=%d samples=%d\r\n", ADC_CHANNEL, ADC_PIN, SAMPLE_RATE, ADC_NUM_SAMPLES);

    capture_samples(dma_chan, &cfg, warmup_buffer, ADC_WARMUP_SAMPLES);
    capture_samples(dma_chan, &cfg, sample_buffer, ADC_NUM_SAMPLES);

    printf("BEGIN %d\r\n", ADC_NUM_SAMPLES);
    for (size_t i = 0; i < ADC_NUM_SAMPLES; i++) {
        printf("%04u\r\n", sample_buffer[i] & 0x0FFF);
    }
    printf("END\r\n");

    while (true) {
        tight_loop_contents();
    }
}
