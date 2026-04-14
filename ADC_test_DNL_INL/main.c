#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

#define ADC_CHANNEL 0
#define ADC_PIN 26
#define SAMPLE_RATE 50000
#define RAMP_PERIOD_MS 1000
#define CAPTURE_PERIODS 4
#define ADC_NUM_SAMPLES (((SAMPLE_RATE * RAMP_PERIOD_MS) / 1000u) * CAPTURE_PERIODS)
#define CAPTURE_DURATION_MS ((ADC_NUM_SAMPLES * 1000u) / SAMPLE_RATE)
#define ADC_WARMUP_SAMPLES 256
#define UART_SETTLE_DELAY_MS 250
#define ADC_BITS 12
#define ADC_CODE_COUNT (1u << ADC_BITS)
#define ADC_MAX_CODE ((1u << ADC_BITS) - 1u)
uint16_t adc_sample_buffer[ADC_NUM_SAMPLES];
uint16_t warmup_buffer[ADC_WARMUP_SAMPLES];
uint32_t adc_histogram[ADC_CODE_COUNT];
uint16_t adc_min_code;
uint16_t adc_max_seen_code;
uint32_t adc_zero_code_hits;

static bool capture_histogram(int dma_chan, dma_channel_config *cfg)
{
    dma_channel_configure(dma_chan, cfg,
                          adc_sample_buffer,
                          &adc_hw->fifo,
                          ADC_NUM_SAMPLES,
                          false);

    adc_fifo_drain();
    dma_start_channel_mask(1u << dma_chan);
    adc_run(true);
    dma_channel_wait_for_finish_blocking(dma_chan);
    adc_run(false);

    memset(adc_histogram, 0, sizeof(adc_histogram));
    adc_min_code = ADC_MAX_CODE;
    adc_max_seen_code = 0u;
    adc_zero_code_hits = 0u;

    for (size_t sample_index = 0; sample_index < ADC_NUM_SAMPLES; sample_index++)
    {
        uint16_t sample = adc_sample_buffer[sample_index] & ADC_MAX_CODE;
        adc_histogram[sample]++;
        if (sample < adc_min_code)
        {
            adc_min_code = sample;
        }
        if (sample > adc_max_seen_code)
        {
            adc_max_seen_code = sample;
        }
        if (sample == 0u)
        {
            adc_zero_code_hits++;
        }
    }

    adc_fifo_drain();

    return true;
}

static void capture_warmup(int dma_chan, dma_channel_config *cfg)
{
    dma_channel_configure(dma_chan, cfg,
                          warmup_buffer,
                          &adc_hw->fifo,
                          ADC_WARMUP_SAMPLES,
                          true);

    adc_run(true);
    dma_channel_wait_for_finish_blocking(dma_chan);
    adc_run(false);
    adc_fifo_drain();
}

int main()
{
    stdio_init_all();
    sleep_ms(UART_SETTLE_DELAY_MS);

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

    printf("# RP2350 ADC DNL/INL capture\r\n");
    printf("# format=histogram input=ramp_0_to_3v3 channel=%d gpio=%d sample_rate=%d ramp_period_ms=%d periods=%d samples=%d adc_bits=%d\r\n",
           ADC_CHANNEL,
           ADC_PIN,
           SAMPLE_RATE,
           RAMP_PERIOD_MS,
           CAPTURE_PERIODS,
           ADC_NUM_SAMPLES,
           ADC_BITS);
    printf("# adc_max_code=%u\r\n", ADC_MAX_CODE);
    printf("# capture_duration_ms=%u\r\n", CAPTURE_DURATION_MS);
    printf("# note=capture_stage_is_silent_until_transmit\r\n");
    printf("# state=warmup\r\n");
    fflush(stdout);

    capture_warmup(dma_chan, &cfg);
    printf("# state=capture\r\n");
    fflush(stdout);
    bool capture_ok = capture_histogram(dma_chan, &cfg);

    if (!capture_ok)
    {
        printf("# state=overrun\r\n");
        printf("# error=histogram_overrun\r\n");
        fflush(stdout);
        while (true)
        {
            tight_loop_contents();
        }
    }

    printf("# state=transmit\r\n");
    printf("# sample_min_code=%u\r\n", adc_min_code);
    printf("# sample_max_code=%u\r\n", adc_max_seen_code);
    printf("# sample_zero_code_hits=%lu\r\n", (unsigned long)adc_zero_code_hits);
    fflush(stdout);
    printf("BEGIN_HIST %u %u\r\n", ADC_CODE_COUNT, ADC_NUM_SAMPLES);
    for (size_t code = 0; code < ADC_CODE_COUNT; code++)
    {
        printf("%lu\r\n", (unsigned long)adc_histogram[code]);
    }
    printf("END\r\n");
    printf("# state=done\r\n");
    fflush(stdout);

    while (true)
    {
        tight_loop_contents();
    }
}
