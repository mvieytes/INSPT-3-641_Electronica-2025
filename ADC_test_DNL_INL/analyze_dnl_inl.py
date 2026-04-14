import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


ADC_BITS = 12
ADC_CODES = 1 << ADC_BITS
DEFAULT_COMPARE_FIRST_VOLTAGE = 0.05
DEFAULT_COMPARE_LAST_VOLTAGE = 3.25


def parse_float_locale(value: str) -> float:
    try:
        return float(value.replace(",", "."))
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"Valor de tension invalido: {value!r}") from exc


def load_capture(path: Path, adc_codes: int) -> tuple[str, np.ndarray, np.ndarray | None]:
    metadata = {}
    numeric_lines = []

    for line_number, raw_line in enumerate(path.read_text(encoding="ascii").splitlines(), start=1):
        line = raw_line.strip()
        if not line:
            continue
        if line.startswith("#"):
            key, separator, value = line[1:].partition("=")
            if separator == "=":
                normalized_key = key.strip()
                normalized_value = value.strip()
                if normalized_key == "format":
                    normalized_value = normalized_value.split()[0]
                metadata.setdefault(normalized_key, normalized_value)
            continue

        try:
            numeric_lines.append(int(line))
        except ValueError as exc:
            raise ValueError(f"Linea {line_number}: valor no numerico: {line!r}") from exc

    if not numeric_lines:
        raise ValueError("No se encontraron datos validos en el archivo de entrada")

    if metadata.get("format") == "histogram":
        histogram = np.asarray(numeric_lines, dtype=np.int64)
        if histogram.size != adc_codes:
            raise ValueError(f"Se esperaban {adc_codes} bins de histograma, pero se encontraron {histogram.size}")
        return "histogram", histogram, None

    samples = np.asarray(numeric_lines, dtype=np.int32)
    if np.any(samples < 0) or np.any(samples >= adc_codes):
        invalid_index = int(np.flatnonzero((samples < 0) | (samples >= adc_codes))[0])
        raise ValueError(f"Linea de datos {invalid_index + 1}: muestra fuera de rango: {int(samples[invalid_index])}")

    histogram = np.bincount(samples, minlength=adc_codes)
    return "samples", histogram, samples


def detect_sweeps(samples: np.ndarray, adc_codes: int) -> tuple[np.ndarray, list[tuple[int, int, int, int]], np.ndarray]:
    reset_threshold = -(adc_codes // 2)
    reset_indices = np.flatnonzero(np.diff(samples) < reset_threshold)

    starts = np.concatenate(([0], reset_indices + 1))
    stops = np.concatenate((reset_indices + 1, [samples.size]))

    coverage = np.zeros(adc_codes, dtype=np.int32)
    sweeps = []
    for start, stop in zip(starts, stops):
        segment = samples[start:stop]
        low_code = int(segment.min())
        high_code = int(segment.max())
        coverage[low_code : high_code + 1] += 1
        sweeps.append((int(start), int(stop), low_code, high_code))

    return coverage, sweeps, reset_indices


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
        default="auto",
        help="Prefijo para guardar histogramas y curvas como PNG. Usa 'auto' para derivarlo del archivo de entrada (default: auto).",
    )
    parser.add_argument(
        "--first-code",
        type=int,
        default=None,
        help="Primer codigo a incluir manualmente. Si se omite se usa 0.",
    )
    parser.add_argument(
        "--last-code",
        type=int,
        default=None,
        help="Ultimo codigo a incluir manualmente. Si se omite se usa el maximo codigo del ADC.",
    )
    parser.add_argument(
        "--first-voltage",
        type=parse_float_locale,
        default=None,
        help="Tension minima de la ventana de analisis en volts. Se convierte a codigo ADC usando --full-scale-voltage.",
    )
    parser.add_argument(
        "--last-voltage",
        type=parse_float_locale,
        default=None,
        help="Tension maxima de la ventana de analisis en volts. Se convierte a codigo ADC usando --full-scale-voltage.",
    )
    parser.add_argument(
        "--full-scale-voltage",
        type=parse_float_locale,
        default=3.3,
        help="Tension de fondo de escala del ADC en volts usada para mapear ventana de tension a codigos (default: 3.3).",
    )
    parser.add_argument(
        "--compare-first-voltage",
        type=parse_float_locale,
        default=DEFAULT_COMPARE_FIRST_VOLTAGE,
        help="Tension minima en volts de la ventana interna comparativa (default: 0.05).",
    )
    parser.add_argument(
        "--compare-last-voltage",
        type=parse_float_locale,
        default=DEFAULT_COMPARE_LAST_VOLTAGE,
        help="Tension maxima en volts de la ventana interna comparativa (default: 3.25).",
    )
    parser.add_argument(
        "--no-compare-window",
        action="store_true",
        help="Desactiva el segundo reporte automatico de la ventana interna comparativa.",
    )
    return parser


def voltage_to_code_window(first_voltage: float | None, last_voltage: float | None, full_scale_voltage: float, adc_codes: int) -> tuple[int | None, int | None]:
    if first_voltage is None and last_voltage is None:
        return None, None

    if first_voltage is None or last_voltage is None:
        raise ValueError("Debes indicar tanto --first-voltage como --last-voltage para usar una ventana por tension")

    if full_scale_voltage <= 0.0:
        raise ValueError("--full-scale-voltage debe ser mayor que cero")

    adc_max_code = adc_codes - 1
    first_code = int(np.ceil((first_voltage / full_scale_voltage) * adc_max_code))
    last_code = int(np.floor((last_voltage / full_scale_voltage) * adc_max_code))
    return first_code, last_code


def detect_code_window(histogram: np.ndarray, first_code: int | None, last_code: int | None) -> tuple[int, int]:
    active_codes = np.flatnonzero(histogram)
    if active_codes.size < 3:
        raise ValueError("No hay suficientes codigos activos para calcular DNL/INL")

    start_code = 0 if first_code is None else first_code
    end_code = histogram.size - 1 if last_code is None else last_code

    if start_code < 0 or end_code >= histogram.size or start_code >= end_code:
        raise ValueError("La ventana de codigos elegida no es valida")

    if end_code - start_code < 3:
        raise ValueError("La ventana de codigos es demasiado chica para calcular DNL/INL")

    return start_code, end_code


def compute_dnl_inl(histogram: np.ndarray, coverage: np.ndarray, start_code: int, end_code: int) -> tuple[np.ndarray, np.ndarray, float, np.ndarray]:
    selected_codes = np.arange(start_code, end_code + 1, dtype=np.int32)
    if selected_codes.size == 0:
        raise ValueError("No hay codigos dentro de la ventana elegida")

    valid_codes = selected_codes[coverage[selected_codes] > 0]
    if valid_codes.size == 0:
        raise ValueError("No hay codigos cubiertos por la rampa dentro de la ventana elegida")

    ideal_count = float(histogram[valid_codes].sum()) / float(coverage[valid_codes].sum())
    if ideal_count <= 0.0:
        raise ValueError("El conteo ideal resulto nulo; revisa el barrido de entrada")

    dnl = np.full(histogram.size, np.nan, dtype=np.float64)
    dnl[valid_codes] = histogram[valid_codes] / (ideal_count * coverage[valid_codes]) - 1.0

    inl = np.full(histogram.size, np.nan, dtype=np.float64)
    cumulative = np.cumsum(dnl[valid_codes])
    cumulative -= np.mean(cumulative)
    inl[valid_codes] = cumulative

    return dnl, inl, ideal_count, valid_codes


def print_capture_overview(capture_format: str, sample_count: int, sweeps: list[tuple[int, int, int, int]], reset_indices: np.ndarray) -> None:
    print("Analisis DNL/INL por histograma")
    print(f"Formato de captura: {capture_format}")
    print(f"Muestras totales: {sample_count}")
    if capture_format == "samples":
        print(f"Resets detectados: {reset_indices.size}")
        print(f"Barridos detectados: {len(sweeps)}")


def print_report(label: str, histogram: np.ndarray, coverage: np.ndarray, dnl: np.ndarray, inl: np.ndarray, ideal_count: float, start_code: int, end_code: int, used_codes: np.ndarray) -> None:
    used_histogram = histogram[used_codes]
    used_dnl = dnl[used_codes]
    used_inl = inl[used_codes]
    used_coverage = coverage[used_codes]

    print()
    print(f"[{label}]")
    print(f"Rango detectado: codigo {start_code} a {end_code}")
    print(f"Codigos usados: {used_codes[0]} a {used_codes[-1]} ({used_codes.size} codigos)")
    print(f"Cobertura por codigo: min {used_coverage.min()} barrido(s), max {used_coverage.max()} barrido(s)")
    print(f"Conteo ideal promedio: {ideal_count:.3f} hits/codigo por barrido")
    print(f"Min hits/codigo: {used_histogram.min()}")
    print(f"Max hits/codigo: {used_histogram.max()}")
    print(f"DNL max: {np.nanmax(used_dnl):+.4f} LSB")
    print(f"DNL min: {np.nanmin(used_dnl):+.4f} LSB")
    print(f"DNL abs max: {np.nanmax(np.abs(used_dnl)):.4f} LSB")
    print(f"INL max: {np.nanmax(used_inl):+.4f} LSB")
    print(f"INL min: {np.nanmin(used_inl):+.4f} LSB")
    print(f"INL abs max: {np.nanmax(np.abs(used_inl)):.4f} LSB")


def build_figure(histogram: np.ndarray, dnl: np.ndarray, inl: np.ndarray, coverage: np.ndarray, start_code: int, end_code: int, title_suffix: str | None = None) -> plt.Figure:
    codes = np.arange(histogram.size)
    fig, axes = plt.subplots(4, 1, figsize=(12, 12), sharex=True, constrained_layout=True)
    suffix = "" if not title_suffix else f" ({title_suffix})"

    axes[0].bar(codes, histogram, width=1.0, color="#1f77b4")
    axes[0].axvspan(start_code, end_code, color="#ffcc80", alpha=0.25)
    axes[0].set_ylabel("Hits")
    axes[0].set_title(f"Histograma de codigos{suffix}")
    axes[0].grid(True, alpha=0.25)

    axes[1].plot(codes, dnl, color="#d62728", linewidth=1.0)
    axes[1].axhline(0.0, color="black", linewidth=0.8)
    axes[1].axvspan(start_code, end_code, color="#ffcc80", alpha=0.25)
    axes[1].set_ylabel("DNL [LSB]")
    axes[1].set_title(f"No linealidad diferencial{suffix}")
    axes[1].grid(True, alpha=0.25)

    axes[2].plot(codes, inl, color="#2ca02c", linewidth=1.0)
    axes[2].axhline(0.0, color="black", linewidth=0.8)
    axes[2].axvspan(start_code, end_code, color="#ffcc80", alpha=0.25)
    axes[2].set_ylabel("INL [LSB]")
    axes[2].set_title(f"No linealidad integral{suffix}")
    axes[2].grid(True, alpha=0.25)

    axes[3].step(codes, coverage, where="mid", color="#9467bd", linewidth=1.0)
    axes[3].axvspan(start_code, end_code, color="#ffcc80", alpha=0.25)
    axes[3].set_ylabel("Barridos")
    axes[3].set_xlabel("Codigo ADC")
    axes[3].set_title(f"Cobertura por codigo{suffix}")
    axes[3].grid(True, alpha=0.25)

    return fig


def resolve_save_prefix(input_path: Path, save_prefix: str | None) -> str | None:
    if save_prefix is None:
        return

    if save_prefix == "auto":
        return input_path.with_suffix("").name

    return save_prefix


def maybe_save_figure(fig: plt.Figure, output_path: Path | None) -> None:
    if output_path is None:
        return

    fig.savefig(output_path, dpi=150)
    print(f"Grafico guardado en {output_path}")


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    adc_codes = 1 << args.adc_bits
    if (args.first_code is not None or args.last_code is not None) and (args.first_voltage is not None or args.last_voltage is not None):
        raise ValueError("Usa --first-code/--last-code o --first-voltage/--last-voltage, pero no ambas formas a la vez")

    voltage_first_code, voltage_last_code = voltage_to_code_window(
        args.first_voltage,
        args.last_voltage,
        args.full_scale_voltage,
        adc_codes,
    )

    input_path = Path(args.input)
    capture_format, histogram, samples = load_capture(input_path, adc_codes)
    if capture_format == "histogram":
        coverage = np.ones(adc_codes, dtype=np.int32)
        sweeps = []
        reset_indices = np.asarray([], dtype=np.int32)
        sample_count = int(histogram.sum())
    else:
        assert samples is not None
        coverage, sweeps, reset_indices = detect_sweeps(samples, adc_codes)
        sample_count = int(samples.size)

    requested_first_code = args.first_code if args.first_code is not None else voltage_first_code
    requested_last_code = args.last_code if args.last_code is not None else voltage_last_code
    start_code, end_code = detect_code_window(histogram, requested_first_code, requested_last_code)

    analyses: list[tuple[str, int, int]] = []
    primary_label = "Escala completa" if requested_first_code is None and requested_last_code is None else "Ventana solicitada"
    analyses.append((primary_label, start_code, end_code))

    if not args.no_compare_window:
        compare_first_code, compare_last_code = voltage_to_code_window(
            args.compare_first_voltage,
            args.compare_last_voltage,
            args.full_scale_voltage,
            adc_codes,
        )
        assert compare_first_code is not None and compare_last_code is not None
        compare_start_code, compare_end_code = detect_code_window(histogram, compare_first_code, compare_last_code)
        if (compare_start_code, compare_end_code) != (start_code, end_code):
            compare_label = f"Ventana interna {args.compare_first_voltage:.3f} V a {args.compare_last_voltage:.3f} V"
            analyses.append((compare_label, compare_start_code, compare_end_code))

    print_capture_overview(capture_format, sample_count, sweeps, reset_indices)

    primary_results: tuple[np.ndarray, np.ndarray, float, np.ndarray] | None = None
    figure_payloads: list[tuple[str, np.ndarray, np.ndarray, int, int]] = []
    for index, (label, analysis_start_code, analysis_end_code) in enumerate(analyses):
        dnl, inl, ideal_count, used_codes = compute_dnl_inl(histogram, coverage, analysis_start_code, analysis_end_code)
        print_report(label, histogram, coverage, dnl, inl, ideal_count, analysis_start_code, analysis_end_code, used_codes)
        figure_payloads.append((label, dnl, inl, analysis_start_code, analysis_end_code))
        if index == 0:
            primary_results = (dnl, inl, ideal_count, used_codes)

    assert primary_results is not None
    dnl, inl, ideal_count, used_codes = primary_results

    save_prefix = resolve_save_prefix(input_path, args.save_prefix)
    if args.show or save_prefix:
        figures: list[plt.Figure] = []
        for index, (label, figure_dnl, figure_inl, figure_start_code, figure_end_code) in enumerate(figure_payloads):
            figure_name_suffix = "full" if index == 0 else "window"
            fig = build_figure(histogram, figure_dnl, figure_inl, coverage, figure_start_code, figure_end_code, label)
            figures.append(fig)
            output_path = Path(f"{save_prefix}_{figure_name_suffix}_dnl_inl.png") if save_prefix else None
            maybe_save_figure(fig, output_path)
        if args.show:
            plt.show()


if __name__ == "__main__":
    main()