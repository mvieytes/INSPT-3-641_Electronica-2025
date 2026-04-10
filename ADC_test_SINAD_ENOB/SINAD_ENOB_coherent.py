import argparse
import numpy as np
from scipy.fft import rfft, rfftfreq
from scipy.signal.windows import blackmanharris
from scipy.optimize import curve_fit
import matplotlib.pyplot as plt
from pathlib import Path


def load_samples(path, adc_min=0, adc_max=4095):
    with open(path, 'r') as f:
        datos = [line.strip() for line in f if line.strip().isdigit()]
    muestras = np.array(datos, dtype=float)
    muestras = muestras[(muestras >= adc_min) & (muestras <= adc_max)]
    return muestras


def _modelo_senoidal_freq_fija(t, amplitud, fase, offset):
    """Modelo senoidal con frecuencia fija (se setea como global)"""
    return amplitud * np.sin(2 * np.pi * _freq_fija * t + fase) + offset


def _modelo_senoidal(t, amplitud, frecuencia, fase, offset):
    """Modelo senoidal con 4 parámetros"""
    return amplitud * np.sin(2 * np.pi * frecuencia * t + fase) + offset

def coherent_fft_analysis(path, fs=50000, adc_min=0, adc_max=4095):
    global _freq_fija  # Para usar en el modelo de ajuste
    
    muestras = load_samples(path, adc_min=adc_min, adc_max=adc_max)
    if len(muestras) == 0:
        raise ValueError('Archivo vacío o sin muestras válidas')

    N = len(muestras)
    freqs = rfftfreq(N, d=1 / fs)
    ventana = blackmanharris(N)
    t = np.arange(N) / fs

    # Centrar para FFT inicial
    x_centered = muestras - np.mean(muestras)

    # FFT sin ventana para detectar coherencia y estimar frecuencia
    spec_raw = rfft(x_centered)
    mag_raw = np.abs(spec_raw)
    mag_raw[0] = 0  # Ignorar DC
    k = np.argmax(mag_raw)
    f_fundamental = freqs[k]

    # Verificar coherencia real: si hay leakage espectral, no es coherent
    potencia_pico = mag_raw[k] ** 2
    potencia_adyacente = 0.0
    for dk in [-2, -1, 1, 2]:
        idx = k + dk
        if 1 <= idx < len(mag_raw):
            potencia_adyacente += mag_raw[idx] ** 2
    
    ratio_leakage = potencia_adyacente / (potencia_pico + 1e-12)
    es_coherent = ratio_leakage < 0.01  # Umbral de 1%
    
    ciclos = N * f_fundamental / fs

    if es_coherent:
        # Muestreo coherente: reconstruir desde FFT
        metodo = "FFT coherente (sin ventana)"
        amplitude = 2 * mag_raw[k] / N
        fase = np.angle(spec_raw[k])
        # Reconstruir usando coseno (FFT da coeficientes de exponencial compleja)
        senal_ajustada = amplitude * np.cos(2 * np.pi * f_fundamental * t + fase) + np.mean(muestras)
    else:
        # Muestreo NO coherente: usar ajuste de curva de 4 parámetros
        metodo = f"Ajuste senoidal 4-param (leakage={ratio_leakage*100:.1f}%)"
        
        amplitud_inicial = (np.max(muestras) - np.min(muestras)) / 2
        offset_inicial = np.mean(muestras)
        
        try:
            parametros, _ = curve_fit(
                _modelo_senoidal,
                t,
                muestras,
                p0=[amplitud_inicial, f_fundamental, 0.0, offset_inicial],
                maxfev=20000,
            )
            amplitude, f_ajustada, fase, offset = parametros
            amplitude = abs(amplitude)
            f_fundamental = f_ajustada  # Usar frecuencia ajustada
            senal_ajustada = _modelo_senoidal(t, amplitude, f_fundamental, fase, offset)
        except Exception:
            # Fallback: usar valores de FFT
            amplitude = 2 * mag_raw[k] / N
            fase = np.angle(spec_raw[k])
            senal_ajustada = amplitude * np.cos(2 * np.pi * f_fundamental * t + fase) + np.mean(muestras)

    # Calcular residuo y métricas
    residuo = muestras - senal_ajustada
    signal_rms = amplitude / np.sqrt(2)
    ruido_dist_rms = np.std(residuo, ddof=1)

    if ruido_dist_rms == 0:
        raise ValueError('Ruido+distorsión nulos; no se puede calcular SINAD')

    sinad = 20 * np.log10(signal_rms / ruido_dist_rms)
    enob = (sinad - 1.76) / 6.02

    # THD y SNR usando FFT del residuo
    spec_residuo = rfft(residuo - np.mean(residuo))
    mag_residuo = np.abs(spec_residuo)
    potencia_residuo = mag_residuo ** 2

    # Identificar armónicos
    harmonic_power = 0.0
    for h in range(2, 10):
        idx_h = h * k
        if idx_h < len(potencia_residuo):
            harmonic_power += potencia_residuo[idx_h]

    harmonic_rms = np.sqrt(2 * harmonic_power) / N
    thd_ratio = harmonic_rms / (amplitude + 1e-12)
    thd_db = 20 * np.log10(thd_ratio + 1e-12)

    potencia_residuo_total = np.sum(potencia_residuo[1:])
    potencia_ruido_puro = max(0, potencia_residuo_total - harmonic_power)
    ruido_puro_rms = np.sqrt(2 * potencia_ruido_puro) / N
    snr_db = 20 * np.log10(signal_rms / (ruido_puro_rms + 1e-12))

    # Guardar espectro
    # FFT con ventana para visualización
    spec_vis = rfft(x_centered * ventana)
    mag_vis = np.abs(spec_vis)
    
    out_base = Path(path).with_suffix('')
    try:
        plt.figure(figsize=(8, 4))
        plt.plot(freqs, 20 * np.log10(mag_vis + 1e-12))
        plt.xlabel('Frecuencia (Hz)')
        plt.ylabel('Magnitud (dB)')
        plt.title(f'Espectro - {metodo}')
        plt.grid(True)
        plt.tight_layout()
        plt.savefig(str(out_base) + '_spectrum_coherent.png', dpi=200)
        plt.close()
    except Exception:
        pass

    return {
        'n_samples': N,
        'metodo': metodo,
        'ciclos': N * f_fundamental / fs,
        'fundamental_freq': f_fundamental,
        'SINAD_dB': sinad,
        'THD_dB': thd_db,
        'SNR_dB': snr_db,
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

    print(f"--- Resultados ({res['metodo']}) ---")
    print(f"Muestras procesadas: {res['n_samples']}")
    print(f"Ciclos detectados: {res['ciclos']:.2f}")
    print(f"Frecuencia fundamental: {res['fundamental_freq']:.2f} Hz")
    print(f"SINAD: {res['SINAD_dB']:.2f} dB")
    print(f"THD:   {res['THD_dB']:.2f} dB")
    print(f"SNR:   {res['SNR_dB']:.2f} dB")
    print(f"ENOB:  {res['ENOB_bits']:.2f} bits")
    print(f"Espectro guardado en: {res['spectrum_file']}")

if __name__ == '__main__':
    main()
