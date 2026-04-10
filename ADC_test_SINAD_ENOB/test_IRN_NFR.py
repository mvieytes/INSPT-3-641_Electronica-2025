import numpy as np
import matplotlib.pyplot as plt

# Cargar datos: primera columna = código ADC, segunda = cantidad de ocurrencias
data = np.loadtxt("samples_IRN_NFR.txt")
codes = data[:, 0].astype(int)
counts = data[:, 1].astype(int)

# Parámetros del ADC
adc_bits = 12
Vfs = 3.3
lsb = Vfs / (2**adc_bits)

# --- Input-Referred Noise ---
# Media ponderada
mean_code = np.sum(codes * counts) / np.sum(counts)

# Desviación estándar ponderada en LSB
sigma_adc = np.sqrt(np.sum(((codes - mean_code) ** 2) * counts) / np.sum(counts))

# Ruido referido a la entrada en LSB RMS
Vnoise_rms_lsb = sigma_adc

# --- Noise-Free Resolution ---
# Ruido pico a pico en LSB (máx - mín de los códigos presentes)
noise_pp = np.max(codes) - np.min(codes)

# NFR en bits
NFR = adc_bits - np.log2(noise_pp)

# --- Resultados ---
total_points = np.sum(counts)

print(f"Total muestras: {total_points}")
print(f"Input-Referred Noise: {Vnoise_rms_lsb:.3f} LSB RMS")
print(f"Noise-Free Resolution: {NFR:.2f} bits")

# --- Histograma de muestras ---
plt.figure()
plt.bar(codes, counts, edgecolor='black')
plt.xlabel("Valor ADC (LSB)")
plt.ylabel("Número de ocurrencias")
plt.title("Histograma de muestras ADC")
plt.grid(True)

# Cuadro con resultados
str_box = (f"Total muestras: {total_points}\n"
           f"Input-Referred Noise: {Vnoise_rms_lsb:.3f} LSB RMS\n"
           f"Noise-Free Resolution: {NFR:.2f} bits")

plt.gcf().text(0.65, 0.6, str_box,
               bbox=dict(facecolor=(0.9, 0.9, 0.9), alpha=0.8),
               fontsize=10)

plt.show()
