import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


ADC_BITS = 12
ADC_CODES = 1 << ADC_BITS


def build_parser():
    parser = argparse.ArgumentParser(
        description="Analiza muestras del ADC tomadas con una rampa lenta y estima DNL e INL por histograma."
    )
    parser.add_argument("input", help="Archivo de entrada con una muestra por linea")
    parser.add_argument(
        "--adc-bits",
        type=int,
        default=ADC_BITS,
        help="Resolucion del ADC en bits (default: 12)",
    )
    parser.add_argument(
        "--show",
        action="store_true",
        help="Muestra los graficos en pantalla.",
    )
    parser.add_argument(
        "--save-prefix",
        default=None,
        help="Prefijo para guardar histogramas y curvas como PNG.",
    )
    parser.add_argument(
        "--first-code",
        type=int,
        default=None,
        help="Primer codigo a incluir manualmente. Si se omite se detecta automaticamente.",
    )
    parser.add_argument(
        "--last-code",
        type=int,
        default=None,
        help="Ultimo codigo a incluir manualmente. Si se omite se detecta automaticamente.",
    )
    return parser


def load_samples(path: Path, adc_codes: int) -> np.ndarray:
    values = []
    for line_number, raw_line in enumerate(path.read_text(encoding="ascii").splitlines(), start=1):
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        try:
            value = int(line)
        except ValueError as exc:
            raise ValueError(f"Linea {line_number}: valor no numerico: {line!r}") from exc
        if value < 0 or value >= adc_codes:
            raise ValueError(f"Linea {line_number}: muestra fuera de rango: {value}")
        values.append(value)

    if not values:
        raise ValueError("No se encontraron muestras validas en el archivo de entrada")

    return np.asarray(values, dtype=np.int32)


def detect_code_window(histogram: np.ndarray, first_code: int | None, last_code: int | None) -> tuple[int, int]:
    active_codes = np.flatnonzero(histogram)
    if active_codes.size < 3:
        raise ValueError("No hay suficientes codigos activos para calcular DNL/INL")

    detected_first = int(active_codes[0])
    detected_last = int(active_codes[-1])

    start_code = detected_first if first_code is None else first_code
    end_code = detected_last if last_code is None else last_code

    if start_code < 0 or end_code >= histogram.size or start_code >= end_code:
        raise ValueError("La ventana de codigos elegida no es valida")

    if end_code - start_code < 3:
        raise ValueError("La ventana de codigos es demasiado chica para calcular DNL/INL")

    return start_code, end_code


def compute_dnl_inl(histogram: np.ndarray, start_code: int, end_code: int) -> tuple[np.ndarray, np.ndarray, float, np.ndarray]:
    interior_codes = np.arange(start_code + 1, end_code, dtype=np.int32)
    if interior_codes.size == 0:
        raise ValueError("No hay codigos interiores luego de excluir los extremos")

    ideal_count = float(histogram[interior_codes].sum()) / float(interior_codes.size)
    if ideal_count <= 0.0:
        raise ValueError("El conteo ideal resulto nulo; revisa el barrido de entrada")

    dnl = np.full(histogram.size, np.nan, dtype=np.float64)
    dnl[interior_codes] = histogram[interior_codes] / ideal_count - 1.0

    inl = np.full(histogram.size, np.nan, dtype=np.float64)
    cumulative = np.cumsum(dnl[interior_codes])
    cumulative -= np.mean(cumulative)
    inl[interior_codes] = cumulative

    return dnl, inl, ideal_count, interior_codes


def print_report(samples: np.ndarray, histogram: np.ndarray, dnl: np.ndarray, inl: np.ndarray, ideal_count: float, start_code: int, end_code: int, interior_codes: np.ndarray) -> None:
    used_histogram = histogram[interior_codes]
    used_dnl = dnl[interior_codes]
    used_inl = inl[interior_codes]

    print("Analisis DNL/INL por histograma")
    print(f"Muestras totales: {samples.size}")
    print(f"Rango detectado: codigo {start_code} a {end_code}")
    print(f"Codigos internos usados: {interior_codes[0]} a {interior_codes[-1]} ({interior_codes.size} codigos)")
    print(f"Conteo ideal promedio: {ideal_count:.3f} hits/codigo")
    print(f"Min hits/codigo: {used_histogram.min()}")
    print(f"Max hits/codigo: {used_histogram.max()}")
    print(f"DNL max: {np.nanmax(used_dnl):+.4f} LSB")
    print(f"DNL min: {np.nanmin(used_dnl):+.4f} LSB")
    print(f"DNL abs max: {np.nanmax(np.abs(used_dnl)):.4f} LSB")
    print(f"INL max: {np.nanmax(used_inl):+.4f} LSB")
    print(f"INL min: {np.nanmin(used_inl):+.4f} LSB")
    print(f"INL abs max: {np.nanmax(np.abs(used_inl)):.4f} LSB")


def build_figure(histogram: np.ndarray, dnl: np.ndarray, inl: np.ndarray, start_code: int, end_code: int) -> plt.Figure:
    codes = np.arange(histogram.size)
    fig, axes = plt.subplots(3, 1, figsize=(12, 10), sharex=True, constrained_layout=True)

    axes[0].bar(codes, histogram, width=1.0, color="#1f77b4")
    axes[0].axvspan(start_code, end_code, color="#ffcc80", alpha=0.25)
    axes[0].set_ylabel("Hits")
    axes[0].set_title("Histograma de codigos")
    axes[0].grid(True, alpha=0.25)

    axes[1].plot(codes, dnl, color="#d62728", linewidth=1.0)
    axes[1].axhline(0.0, color="black", linewidth=0.8)
    axes[1].axvspan(start_code, end_code, color="#ffcc80", alpha=0.25)
    axes[1].set_ylabel("DNL [LSB]")
    axes[1].set_title("No linealidad diferencial")
    axes[1].grid(True, alpha=0.25)

    axes[2].plot(codes, inl, color="#2ca02c", linewidth=1.0)
    axes[2].axhline(0.0, color="black", linewidth=0.8)
    axes[2].axvspan(start_code, end_code, color="#ffcc80", alpha=0.25)
    axes[2].set_ylabel("INL [LSB]")
    axes[2].set_xlabel("Codigo ADC")
    axes[2].set_title("No linealidad integral")
    axes[2].grid(True, alpha=0.25)

    return fig


def maybe_save_figure(fig: plt.Figure, prefix: str | None) -> None:
    if prefix is None:
        return
    output_path = Path(f"{prefix}_dnl_inl.png")
    fig.savefig(output_path, dpi=150)
    print(f"Grafico guardado en {output_path}")


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    adc_codes = 1 << args.adc_bits
    samples = load_samples(Path(args.input), adc_codes)
    histogram = np.bincount(samples, minlength=adc_codes)
    start_code, end_code = detect_code_window(histogram, args.first_code, args.last_code)
    dnl, inl, ideal_count, interior_codes = compute_dnl_inl(histogram, start_code, end_code)

    print_report(samples, histogram, dnl, inl, ideal_count, start_code, end_code, interior_codes)

    if args.show or args.save_prefix:
        fig = build_figure(histogram, dnl, inl, start_code, end_code)
        maybe_save_figure(fig, args.save_prefix)
        if args.show:
            plt.show()


if __name__ == "__main__":
    main()