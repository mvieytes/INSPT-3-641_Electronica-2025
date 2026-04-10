import argparse
import math
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


def build_parser():
    parser = argparse.ArgumentParser(
        description="Calcula Input-Referred Noise y Noise-Free Resolution a partir de un archivo de muestras ADC."
    )
    parser.add_argument("input", help="Archivo TXT con una muestra ADC por linea")
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


def load_samples(path):
    values = []
    for raw_line in Path(path).read_text(encoding="ascii").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        values.append(int(line, 10))

    if not values:
        raise ValueError("El archivo no contiene muestras validas")

    return np.asarray(values, dtype=np.float64)


def calculate_metrics(samples, bits, vref):
    full_scale_codes = float(1 << bits)
    lsb_volts = vref / full_scale_codes

    mean_code = float(np.mean(samples))
    std_code = float(np.std(samples, ddof=1)) if len(samples) > 1 else 0.0
    min_code = int(np.min(samples))
    max_code = int(np.max(samples))
    observed_pp_codes = max_code - min_code
    gaussian_pp_codes = 6.6 * std_code

    inr_rms_lsb = std_code
    inr_rms_volts = inr_rms_lsb * lsb_volts

    observed_noise_free_bits = math.inf if observed_pp_codes == 0 else bits - math.log2(observed_pp_codes)
    gaussian_noise_free_bits = math.inf if gaussian_pp_codes <= 0 else bits - math.log2(gaussian_pp_codes)

    return {
        "count": len(samples),
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


def plot_histogram(samples, metrics, save_path):
    figure, axis = plt.subplots(figsize=(9, 5.5), constrained_layout=True)

    bins = np.arange(samples.min() - 0.5, samples.max() + 1.5, 1.0)
    axis.hist(samples, bins=bins, color="#0f766e", edgecolor="#134e4a", alpha=0.9)
    axis.axvline(metrics["mean_code"], color="#b45309", linestyle="--", linewidth=2, label="Media")

    axis.set_title("Histograma de codigos ADC")
    axis.set_xlabel("Codigo ADC")
    axis.set_ylabel("Frecuencia")
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
    samples = load_samples(args.input)
    metrics = calculate_metrics(samples, bits=args.bits, vref=args.vref)

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

    figure = plot_histogram(samples, metrics, args.save_plot)
    print(f"Histograma guardado en: {args.save_plot}")

    if args.show:
        plt.show()
    else:
        plt.close(figure)


if __name__ == "__main__":
    main()