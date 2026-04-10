import argparse
import numpy as np
from scipy.fft import rfft, rfftfreq
from scipy.optimize import curve_fit
from scipy.signal.windows import blackmanharris

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

    # 2. Pre-procesamiento
    muestras -= np.mean(muestras)

    # FFT solo para estimar la frecuencia inicial de la fundamental.
    ventana = blackmanharris(n)
    muestras_ventaneadas = muestras * ventana

    espectro = np.abs(rfft(muestras_ventaneadas))
    potencia = espectro**2
    idx_fundamental = np.argmax(potencia[1:]) + 1
    freqs = rfftfreq(n, d=1 / fs)
    frecuencia_inicial = freqs[idx_fundamental]

    # 3. Ajuste senoidal: más robusto cuando no hay muestreo coherente exacto.
    tiempo = np.arange(n) / fs
    amplitud_inicial = (np.max(muestras) - np.min(muestras)) / 2
    offset_inicial = 0.0

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

    print(f"--- Resultados del Análisis ---")
    print(f"Muestras procesadas: {n}")
    if descartadas:
        print(f"Muestras descartadas por fuera de rango [{adc_min}, {adc_max}]: {descartadas}")
    print(f"Frecuencia Fundamental: {frecuencia_fundamental:.2f} Hz")
    print(f"SINAD: {sinad:.2f} dB")
    print(f"ENOB:  {enob:.2f} bits")


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
