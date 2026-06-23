#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware.h"
#include "hardware/adc.h"

/* Se puede dejar ambas comentadas p/12bits, o elegir entre 10 u 8 bits */
//#define ADC_8BITS
//#define ADC_10BITS

/* Si está instalada la extensión Teleplot, para ver las gráficas de la salida */
#define USA_TELEPLOT

#define LAPSO_MEDICION          (10)
uint32_t lapso;

/* PROM MOVIL */
#define CANT_VAL_PROM_MOVIL     (8)
uint16_t valores_adc[CANT_VAL_PROM_MOVIL];
uint8_t indice;
uint32_t acum_med;

float promedio_movil(uint16_t muestra);

/* IIR */
#define ALPHA                   (0.1f)
float salida_iir = 0;

float filtro_iir(float val_inicial, float val_actual);

/* FIR */
#define CANT_VAL_FIR            (8)
float coef_h[CANT_VAL_FIR] = { (1.0 / CANT_VAL_FIR),(1.0 / CANT_VAL_FIR), (1.0 / CANT_VAL_FIR), (1.0 / CANT_VAL_FIR), (1.0 / CANT_VAL_FIR), (1.0 / CANT_VAL_FIR), (1.0 / CANT_VAL_FIR), (1.0 / CANT_VAL_FIR) };
float valores_adc_fir[CANT_VAL_FIR] = { 0 };
uint8_t indice_fir = 0;
float salida_fir = 0;

float filtro_fir(float val_actual);

int main() {
    stdio_init_all();

    /* Inicializa buffer y acumulador para promedio movil */
    for (int i = 0; i < CANT_VAL_PROM_MOVIL; i++) {
        valores_adc[i] = 0;
    }
    acum_med = 0;

    adc_init();
    adc_gpio_init(ADC_0_GPIO);
    adc_select_input(ADC_CHANNEL_0);

    lapso = to_ms_since_boot(get_absolute_time());

    while (true) {
        if (lapso <= to_ms_since_boot(get_absolute_time())) {
            lapso = lapso + LAPSO_MEDICION;
            uint16_t raw = adc_read();                 // leer ADC (12 bits)
#ifdef ADC_10BITS
            raw >>= 2;
#endif
#ifdef ADC_8BITS
            raw >>= 4;
#endif
            /* Promedio movil */
            float prom_med = promedio_movil(raw);
            float vin_med = prom_med * 3.3 / 4096;
            /* Filtro IIR */
            salida_iir = filtro_iir(salida_iir, (float)raw);
            /* Filtro FIR */
            salida_fir = filtro_fir((float)raw);
#ifndef USA_TELEPLOT
            printf("Cuentas: %4d - Prom: %4.2f - Vin: %1.2fV - IIR: %4.2f - FIR: %4.2f\n", raw, prom_med, vin_med, salida_iir, salida_fir);
#endif
#ifdef USA_TELEPLOT
            printf(">Prom Mov.:%f\n", prom_med);
            printf(">IIR:%f\n", salida_iir);
            printf(">FIR:%f\n", salida_fir);
#endif
        }
    }
}

/*
Obtiene el promedio de las últimas "CANT_VAL_PROM_MOVIL" (8 en este ejemplo) muestras que se toman
cada "LAPSO_MEDICION" (10mS para el ejemplo). En el buffer se descarta la muestra más antigua,
se reemplaza por la actual y se promedian las (hasta CANT_VAL_PROM_MOVIL) últimas obtenidas.
Se obtiene un promedio cada LAPSO_MEDICION
*/
float promedio_movil(uint16_t muestra) {
    static uint8_t divisor = 1;
    float promedio;

    // Restar el valor que sale de la ventana
    acum_med -= valores_adc[indice];
    // Guardar la nueva muestra
    valores_adc[indice] = muestra;
    // Sumarla al acumulador
    acum_med += muestra;
    // Avanzar índice circular
    indice++;
    // Debe ir entre 0 y (CANT_VAL_PROM_MOVIL - 1)
    indice %= CANT_VAL_PROM_MOVIL;
    // Calculo promedio
    promedio = ((float)(acum_med) / divisor);
    // Avanzar divisor, si llega a CANT_VAL_PROM_MOVIL queda así
    if (divisor < CANT_VAL_PROM_MOVIL)
        divisor++;
    // Devolver promedio
    return (promedio);
}

/*
El filtro IIR implementado es:
    y[n]=α⋅x[n]+(1-α)⋅y[n-1]
Donde:
    x[n]: muestra de ADC.
    y[n]: salida filtrada.
    α: coeficiente de suavizado (entre 0 y 1, ej. 0.1).
    Valores menores de α filtro más lento y suavizado, mayores más rápido y menos suave
*/
float filtro_iir(float val_inicial, float val_actual) {
    return (ALPHA * val_actual + (1.0f - ALPHA) * val_inicial);
}

/*
Un FIR (Finite Impulse Response) se implementa con:
    y[n]=∑(de (k=0) a (N-1)) de〖h[k]⋅x[n-k]〗
donde:
    h[k]: coeficientes del filtro (impulso respuesta).
    x[n−k]: muestras de entrada (ADC).
    N: orden del filtro (cantidad de coeficientes).

    El ejemplo implementa un promedio móvil usando FIR
*/
float filtro_fir(float val_actual) {
    float y = 0.0f;

    // Guardar nueva muestra en buffer circular
    valores_adc_fir[indice_fir] = val_actual;

    // Convolución: y[n] = sum(h[k] * x[n-k])
    int j = indice_fir;
    for (int k = 0; k < CANT_VAL_FIR; k++) {
        y += coef_h[k] * valores_adc_fir[j];
        j = (j - 1 + CANT_VAL_FIR) % CANT_VAL_FIR; // retroceder en buffer circular
    }

    // Avanzar índice
    indice_fir = (indice_fir + 1) % CANT_VAL_FIR;

    return y;
}
