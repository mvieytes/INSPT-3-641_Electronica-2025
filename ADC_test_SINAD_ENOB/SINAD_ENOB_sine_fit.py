import argparse
import numpy as np
from scipy.fft import rfft, rfftfreq
from scipy.optimize import curve_fit
from scipy.signal.windows import blackmanharris
import matplotlib.pyplot as plt
from pathlib import Path

def _modelo_senoidal(t, amplitud, frecuencia, fase, offset):
    return amplitud * np.sin(2 * np.pi * frecuencia * t + fase) + offset


def calcular_sinad_enob(archivo_datos, fs=50000, adc_min=0, adc_max=4095):
    # 1. Cargar datos
    try:
        with open(archivo_datos, 'r') as f:
            # Filtramos solo líneas que tengan números (por si hay texto como "DONE")
            datos = [line.strip() for line in f if line.strip().isdigit()]
            muestras = np.array(datos, dtype=float)
    except Exception as e:
        return f"Error al leer el archivo: {e}"

    if len(muestras) == 0:
        return "El archivo está vacío"

    muestras_validas = muestras[(muestras >= adc_min) & (muestras <= adc_max)]
    descartadas = len(muestras) - len(muestras_validas)
    muestras = muestras_validas

    n = len(muestras)
    if n < 32:
        return "No hay suficientes muestras válidas para calcular SINAD/ENOB"

    # 2. Pre-procesamiento: NO restamos media aquí, dejamos que el ajuste encuentre el offset
    muestras_centradas = muestras - np.mean(muestras)  # Solo para FFT inicial

    # FFT solo para estimar la frecuencia inicial de la fundamental.
    ventana = blackmanharris(n)
    muestras_ventaneadas = muestras_centradas * ventana

    espectro = np.abs(rfft(muestras_ventaneadas))
    potencia = espectro**2
    idx_fundamental = np.argmax(potencia[1:]) + 1
    freqs = rfftfreq(n, d=1 / fs)
    frecuencia_inicial = freqs[idx_fundamental]

    # 3. Ajuste senoidal: más robusto cuando no hay muestreo coherente exacto.
    tiempo = np.arange(n) / fs
    amplitud_inicial = (np.max(muestras) - np.min(muestras)) / 2
    offset_inicial = np.mean(muestras)  # Mejor estimación inicial del offset

    try:
        parametros, _ = curve_fit(
            _modelo_senoidal,
            tiempo,
            muestras,
            p0=[amplitud_inicial, frecuencia_inicial, 0.0, offset_inicial],
            maxfev=20000,
        )
    except Exception as e:
        return f"Error al ajustar la senoidal: {e}"

    amplitud, frecuencia_fundamental, fase, offset = parametros
    senal_ajustada = _modelo_senoidal(tiempo, amplitud, frecuencia_fundamental, fase, offset)
    residuo = muestras - senal_ajustada

    senal_rms = abs(amplitud) / np.sqrt(2)
    ruido_distorsion_rms = np.std(residuo, ddof=1)
    if ruido_distorsion_rms == 0:
        return "Ruido + distorsión nulos; no se puede calcular SINAD"

    sinad = 20 * np.log10(senal_rms / ruido_distorsion_rms)
    enob = (sinad - 1.76) / 6.02

    # Calcular THD y SNR separados usando FFT del residuo
    residuo_centrado = residuo - np.mean(residuo)
    espectro_residuo = np.abs(rfft(residuo_centrado * ventana))
    potencia_residuo = espectro_residuo ** 2

    # Identificar armónicos (2f, 3f, 4f, ...) dentro del rango de Nyquist
    potencia_armonicos = 0.0
    num_armonicos = 0
    for h in range(2, 10):  # Armónicos 2 a 9
        idx_h = int(round(h * idx_fundamental))
        if idx_h < len(potencia_residuo):
            # Sumar bins cercanos (+/-1) para capturar leakage
            start = max(1, idx_h - 1)
            end = min(len(potencia_residuo), idx_h + 2)
            potencia_armonicos += np.sum(potencia_residuo[start:end])
            num_armonicos += 1

    # THD = sqrt(suma potencia armónicos) / amplitud fundamental
    thd_ratio = np.sqrt(potencia_armonicos) / (espectro[idx_fundamental] + 1e-12)
    thd_db = 20 * np.log10(thd_ratio + 1e-12)

    # SNR = señal / ruido (excluyendo armónicos)
    potencia_ruido = np.sum(potencia_residuo) - potencia_armonicos
    ruido_rms = np.sqrt(max(0, potencia_ruido)) / (n / 2)  # Normalizar
    snr_db = 20 * np.log10(senal_rms / (ruido_rms + 1e-12))

    print(f"--- Resultados del Análisis (Sine Curve Fit) ---")
    print(f"Muestras procesadas: {n}")
    if descartadas:
        print(f"Muestras descartadas por fuera de rango [{adc_min}, {adc_max}]: {descartadas}")
    print(f"Frecuencia Fundamental: {frecuencia_fundamental:.2f} Hz")
    print(f"SINAD: {sinad:.2f} dB")
    print(f"THD:   {thd_db:.2f} dB")
    print(f"SNR:   {snr_db:.2f} dB")
    print(f"ENOB:  {enob:.2f} bits")

    # Guardar gráficos: espectro y ajuste en tiempo
    try:
        fig1 = plt.figure(figsize=(8, 4))
        plt.plot(freqs, 20 * np.log10(espectro + 1e-12))
        plt.xlabel('Frecuencia (Hz)')
        plt.ylabel('Magnitud (dB)')
        plt.title('Espectro (ventana Blackman-Harris)')
        plt.grid(True)
        out_spec = Path(archivo_datos).with_suffix('')
        plt.tight_layout()
        fig1.savefig(str(out_spec) + '_spectrum.png', dpi=200)
        plt.close(fig1)

        fig2 = plt.figure(figsize=(8, 4))
        plt.plot(tiempo, muestras, label='Muestras')
        plt.plot(tiempo, senal_ajustada, label='Senoide ajustada')
        plt.plot(tiempo, residuo, label='Residuo (ruido+distorsión)')
        plt.xlabel('Tiempo (s)')
        plt.ylabel('ADC (units)')
        plt.title('Ajuste en tiempo y residuo')
        plt.legend()
        plt.grid(True)
        plt.tight_layout()
        fig2.savefig(str(out_spec) + '_timedomain.png', dpi=200)
        plt.close(fig2)
        print(f"Gráficos guardados: {out_spec}_spectrum.png, {out_spec}_timedomain.png")
    except Exception as e:
        print(f"No se pudo generar los gráficos: {e}")


def main():
    parser = argparse.ArgumentParser(
        description="Calcula SINAD y ENOB a partir de un archivo de muestras."
    )
    parser.add_argument("archivo_datos", help="Ruta al archivo TXT con una muestra por linea")
    parser.add_argument(
        "fs",
        nargs="?",
        type=float,
        default=50000,
        help="Frecuencia de muestreo en Hz (default: 50000)",
    )
    parser.add_argument(
        "--adc-min",
        type=float,
        default=0,
        help="Valor minimo valido del ADC (default: 0)",
    )
    parser.add_argument(
        "--adc-max",
        type=float,
        default=4095,
        help="Valor maximo valido del ADC (default: 4095 para 12 bits)",
    )
    args = parser.parse_args()

    resultado = calcular_sinad_enob(
        args.archivo_datos,
        fs=args.fs,
        adc_min=args.adc_min,
        adc_max=args.adc_max,
    )
    if isinstance(resultado, str):
        print(resultado)
        raise SystemExit(1)


if __name__ == "__main__":
    main()
