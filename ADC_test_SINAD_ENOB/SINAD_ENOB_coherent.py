import argparse
import numpy as np
from scipy.fft import rfft, rfftfreq
from scipy.signal import hilbert
import matplotlib.pyplot as plt
from pathlib import Path


def load_samples(path, adc_min=0, adc_max=4095):
    with open(path, 'r') as f:
        datos = [line.strip() for line in f if line.strip().isdigit()]
    muestras = np.array(datos, dtype=float)
    muestras = muestras[(muestras >= adc_min) & (muestras <= adc_max)]
    return muestras


def extract_integer_cycles(x):
    # Usar la envolvente analítica para estimar fase y recortar a un número entero de ciclos
    if len(x) < 16:
        raise ValueError('No hay suficientes muestras')

    z = hilbert(x)
    phase = np.unwrap(np.angle(z))
    total_cycles = (phase[-1] - phase[0]) / (2 * np.pi)
    cycles_int = int(np.floor(total_cycles))
    if cycles_int < 1:
        raise ValueError('No se detecta al menos un ciclo completo')

    # Encontrar índices donde la fase cruza múltiplos de 2pi desde el inicio
    start_phase = phase[0]
    boundaries = [start_phase + m * 2 * np.pi for m in range(0, cycles_int + 1)]
    idx = [np.searchsorted(phase, b) for b in boundaries]
    # Asegurar índices válidos
    if idx[0] >= idx[-1] or idx[-1] - idx[0] < 8:
        raise ValueError('No se pudieron localizar límites de ciclo adecuados')

    return x[idx[0]:idx[-1]]


def coherent_fft_analysis(path, fs=50000, adc_min=0, adc_max=4095):
    muestras = load_samples(path, adc_min=adc_min, adc_max=adc_max)
    if len(muestras) == 0:
        raise ValueError('Archivo vacío o sin muestras válidas')

    # centrar
    x = muestras - np.mean(muestras)

    # Extraer conjunto con número entero de ciclos
    try:
        x_coh = extract_integer_cycles(x)
    except Exception as e:
        raise

    N = len(x_coh)
    spec = rfft(x_coh)
    freqs = rfftfreq(N, d=1 / fs)

    # Ignorar DC para localizar la fundamental
    mag = np.abs(spec)
    mag[0] = 0
    k = np.argmax(mag)

    # Amplitud estimada desde la bin k (coherente -> energía en un bin)
    amplitude = 2 * mag[k] / N
    signal_rms = amplitude / np.sqrt(2)

    total_power_time = np.mean(x_coh ** 2)
    signal_power_time = (amplitude ** 2) / 2
    noise_dist_rms = np.sqrt(max(0.0, total_power_time - signal_power_time))

    if noise_dist_rms == 0:
        raise ValueError('Ruido+distorsión nulos; no se puede calcular SINAD')

    sinad = 20 * np.log10(signal_rms / noise_dist_rms)
    enob = (sinad - 1.76) / 6.02

    # Guardar espectro coherente
    out_base = Path(path).with_suffix('')
    try:
        plt.figure(figsize=(8, 4))
        plt.plot(freqs, 20 * np.log10(mag + 1e-12))
        plt.xlabel('Frecuencia (Hz)')
        plt.ylabel('Magnitud (dB)')
        plt.title('Espectro Coherente (sin ventana)')
        plt.grid(True)
        plt.tight_layout()
        plt.savefig(str(out_base) + '_spectrum_coherent.png', dpi=200)
        plt.close()
    except Exception:
        pass

    return {
        'n_samples': len(muestras),
        'n_used': N,
        'fundamental_freq': freqs[k],
        'SINAD_dB': sinad,
        'ENOB_bits': enob,
        'spectrum_file': str(out_base) + '_spectrum_coherent.png',
    }


def main():
    parser = argparse.ArgumentParser(description='Análisis Coherent-FFT para SINAD/ENOB')
    parser.add_argument('archivo_datos')
    parser.add_argument('fs', nargs='?', type=float, default=50000)
    parser.add_argument('--adc-min', type=float, default=0)
    parser.add_argument('--adc-max', type=float, default=4095)
    args = parser.parse_args()

    try:
        res = coherent_fft_analysis(args.archivo_datos, fs=args.fs, adc_min=args.adc_min, adc_max=args.adc_max)
    except Exception as e:
        print(f'Error: {e}')
        raise SystemExit(1)

    print('--- Resultados Coherent-FFT ---')
    print(f"Muestras totales: {res['n_samples']}")
    print(f"Muestras usadas (ciclos enteros): {res['n_used']}")
    print(f"Frecuencia fundamental: {res['fundamental_freq']:.2f} Hz")
    print(f"SINAD: {res['SINAD_dB']:.2f} dB")
    print(f"ENOB:  {res['ENOB_bits']:.2f} bits")
    print(f"Espectro guardado en: {res['spectrum_file']}")


if __name__ == '__main__':
    main()
