import argparse
import math
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


def build_parser():
    parser = argparse.ArgumentParser(
        description="Calcula Input-Referred Noise y Noise-Free Resolution a partir de un archivo con 4096 pares 'codigo_adc ocurrencias'."
    )
    parser.add_argument("input", help="Archivo TXT con 4096 lineas del tipo 'codigo_adc ocurrencias'")
    parser.add_argument(
        "--vref",
        type=float,
        default=3.3,
        help="Voltaje de referencia del ADC en voltios (default: 3.3)",
    )
    parser.add_argument(
        "--bits",
        type=int,
        default=12,
        help="Resolucion nominal del ADC en bits (default: 12)",
    )
    parser.add_argument(
        "--save-plot",
        default="histograma_inr_nfr.png",
        help="Ruta donde guardar el histograma (default: histograma_inr_nfr.png)",
    )
    parser.add_argument(
        "--show",
        action="store_true",
        help="Muestra la figura en pantalla ademas de guardarla",
    )
    return parser


def _parse_numeric_line(line):
    normalized = line.replace(",", " ").replace(";", " ").replace("-", " ")
    parts = normalized.split()
    if not parts:
        return None
    if len(parts) != 2:
        raise ValueError(f"Se esperaba una linea 'codigo ocurrencias' y se recibio: {line}")
    try:
        return tuple(int(part, 10) for part in parts)
    except ValueError as exc:
        raise ValueError(f"Linea invalida: {line}") from exc


def load_histogram(path, bits):
    histogram_size = 1 << bits
    histogram_pairs = []

    for raw_line in Path(path).read_text(encoding="ascii").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue

        parsed = _parse_numeric_line(line)
        if parsed is None:
            continue
        histogram_pairs.append(parsed)

    if histogram_pairs:
        if len(histogram_pairs) != histogram_size:
            raise ValueError(
                f"Se esperaban {histogram_size} lineas codigo-conteo y se recibieron {len(histogram_pairs)}"
            )

        histogram = np.zeros(histogram_size, dtype=np.int64)
        seen_codes = set()
        for code, count in histogram_pairs:
            if code < 0 or code >= histogram_size:
                raise ValueError(f"Codigo ADC fuera de rango: {code}")
            if count < 0:
                raise ValueError(f"Conteo negativo no valido para el codigo {code}")
            if code in seen_codes:
                raise ValueError(f"Codigo ADC repetido en el histograma: {code}")
            seen_codes.add(code)
            histogram[code] = count

        if len(seen_codes) != histogram_size:
            missing_codes = sorted(set(range(histogram_size)) - seen_codes)
            raise ValueError(f"Faltan codigos ADC en el histograma: {missing_codes[:8]}")

        return histogram

    raise ValueError("El archivo no contiene datos validos")



def calculate_metrics(histogram, bits, vref):
    full_scale_codes = float(1 << bits)
    lsb_volts = vref / full_scale_codes

    total_samples = int(histogram.sum())
    if total_samples <= 0:
        raise ValueError("El histograma no contiene ocurrencias")

    codes = np.arange(len(histogram), dtype=np.float64)
    weights = histogram.astype(np.float64)
    mean_code = float(np.dot(codes, weights) / total_samples)

    centered = codes - mean_code
    sum_squared = float(np.dot(centered * centered, weights))
    std_code = math.sqrt(sum_squared / (total_samples - 1)) if total_samples > 1 else 0.0

    nonzero_codes = np.flatnonzero(histogram)
    min_code = int(nonzero_codes[0])
    max_code = int(nonzero_codes[-1])

    observed_pp_codes = max_code - min_code
    gaussian_pp_codes = 6.6 * std_code

    inr_rms_lsb = std_code
    inr_rms_volts = inr_rms_lsb * lsb_volts

    observed_noise_free_bits = math.inf if observed_pp_codes == 0 else bits - math.log2(observed_pp_codes)
    gaussian_noise_free_bits = math.inf if gaussian_pp_codes <= 0 else bits - math.log2(gaussian_pp_codes)

    return {
        "count": total_samples,
        "mean_code": mean_code,
        "std_code": std_code,
        "min_code": min_code,
        "max_code": max_code,
        "observed_pp_codes": observed_pp_codes,
        "gaussian_pp_codes": gaussian_pp_codes,
        "lsb_volts": lsb_volts,
        "inr_rms_lsb": inr_rms_lsb,
        "inr_rms_volts": inr_rms_volts,
        "observed_noise_free_bits": observed_noise_free_bits,
        "gaussian_noise_free_bits": gaussian_noise_free_bits,
    }


def format_bits(value):
    if math.isinf(value):
        return "inf"
    return f"{value:.3f}"


def plot_histogram(histogram, metrics, save_path):
    figure, axis = plt.subplots(figsize=(9, 5.5), constrained_layout=True)

    active_codes = np.flatnonzero(histogram)
    display_start = int(active_codes[0])
    display_end = int(active_codes[-1])
    plot_codes = np.arange(display_start, display_end + 1)

    axis.bar(
        plot_codes,
        histogram[display_start : display_end + 1],
        width=0.95,
        color="#0f766e",
        edgecolor="#134e4a",
        alpha=0.9,
    )
    axis.axvline(metrics["mean_code"], color="#b45309", linestyle="--", linewidth=2, label="Media")

    axis.set_title("Histograma de codigos ADC")
    axis.set_xlabel("Codigo ADC")
    axis.set_ylabel("Ocurrencias")
    axis.set_xlim(display_start - 0.5, display_end + 0.5)
    axis.grid(alpha=0.25)

    summary = "\n".join(
        [
            f"N = {metrics['count']}",
            f"Media = {metrics['mean_code']:.3f} codes",
            f"Sigma = {metrics['std_code']:.4f} LSB_rms",
            f"INR = {metrics['inr_rms_volts'] * 1e6:.2f} uV_rms",
            f"PP obs = {metrics['observed_pp_codes']} codes",
            f"NFR obs = {format_bits(metrics['observed_noise_free_bits'])} bits",
            f"NFR 6.6sigma = {format_bits(metrics['gaussian_noise_free_bits'])} bits",
        ]
    )
    axis.text(
        0.98,
        0.98,
        summary,
        transform=axis.transAxes,
        va="top",
        ha="right",
        fontsize=10,
        bbox={"facecolor": "white", "edgecolor": "#d1d5db", "boxstyle": "round,pad=0.4"},
    )
    axis.legend(loc="upper left")

    figure.savefig(save_path, dpi=150)
    return figure


def main():
    args = build_parser().parse_args()
    histogram = load_histogram(args.input, bits=args.bits)
    metrics = calculate_metrics(histogram, bits=args.bits, vref=args.vref)

    print(f"Muestras               : {metrics['count']}")
    print(f"Media                  : {metrics['mean_code']:.6f} codes")
    print(f"Desvio estandar        : {metrics['std_code']:.6f} LSB_rms")
    print(f"Min / Max              : {metrics['min_code']} / {metrics['max_code']} codes")
    print(f"Ruido p-p observado    : {metrics['observed_pp_codes']} codes")
    print(f"Ruido p-p (6.6 sigma)  : {metrics['gaussian_pp_codes']:.6f} codes")
    print(f"LSB                    : {metrics['lsb_volts'] * 1e6:.6f} uV")
    print(f"INR rms                : {metrics['inr_rms_volts'] * 1e6:.6f} uV_rms")
    print(f"NFR observado          : {format_bits(metrics['observed_noise_free_bits'])} bits")
    print(f"NFR estimado 6.6 sigma : {format_bits(metrics['gaussian_noise_free_bits'])} bits")

    figure = plot_histogram(histogram, metrics, args.save_plot)
    print(f"Histograma guardado en: {args.save_plot}")

    if args.show:
        plt.show()
    else:
        plt.close(figure)


if __name__ == "__main__":
    main()