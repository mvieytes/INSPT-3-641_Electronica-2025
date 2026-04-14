import argparse
import sys
import time
from pathlib import Path

import serial
from serial.tools import list_ports


DEFAULT_TIMEOUT = 30.0
DEFAULT_TIMEOUT_MARGIN = 5.0
DEFAULT_BAUDRATE = 460800
DEFAULT_EXPECTED_COUNT = 4096
DEFAULT_TOTAL_SAMPLES = 200000
DEFAULT_WARMUP_COUNT = 256
DEFAULT_SAMPLE_RATE = 50000
DEFAULT_ADC_BITS = 12


def _build_parser():
    parser = argparse.ArgumentParser(
        description="Captura un bloque de muestras ADC enviadas por el puerto serie del Pico y lo guarda en un TXT."
    )
    parser.add_argument("port", nargs="?", help="Puerto serie, por ejemplo COM5")
    parser.add_argument(
        "--baudrate",
        type=int,
        default=DEFAULT_BAUDRATE,
        help="Baudrate UART (default: 460800)",
    )
    parser.add_argument(
        "--output",
        default="samples.txt",
        help="Archivo de salida (default: samples.txt)",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=None,
        help="Tiempo maximo total en segundos. Si se omite, se estima desde la configuracion del ensayo.",
    )
    parser.add_argument(
        "--timeout-margin",
        type=float,
        default=DEFAULT_TIMEOUT_MARGIN,
        help="Margen extra agregado al timeout estimado, en segundos (default: 5)",
    )
    parser.add_argument(
        "--expected-count",
        type=int,
        default=DEFAULT_EXPECTED_COUNT,
        help="Cantidad esperada de items del bloque UART (default: 4096)",
    )
    parser.add_argument(
        "--total-samples",
        type=int,
        default=DEFAULT_TOTAL_SAMPLES,
        help="Cantidad total de muestras ADC acumuladas por el firmware (default: 200000)",
    )
    parser.add_argument(
        "--sample-rate",
        type=int,
        default=DEFAULT_SAMPLE_RATE,
        help="Frecuencia de muestreo del firmware en muestras/s (default: 50000)",
    )
    parser.add_argument(
        "--warmup-count",
        type=int,
        default=DEFAULT_WARMUP_COUNT,
        help="Cantidad de muestras de calentamiento descartadas por el firmware (default: 256)",
    )
    parser.add_argument(
        "--adc-bits",
        type=int,
        default=DEFAULT_ADC_BITS,
        help="Resolucion del ADC usada para estimar el tiempo de transferencia (default: 12)",
    )
    parser.add_argument(
        "--list-ports",
        action="store_true",
        help="Lista los puertos serie disponibles y termina.",
    )
    return parser


def _list_ports():
    ports = list(list_ports.comports())
    if not ports:
        print("No se encontraron puertos serie.")
        return 0

    print("Puertos serie disponibles:")
    for port in ports:
        description = f" - {port.description}" if port.description else ""
        print(f"{port.device}{description}")
    return 0


def _parse_begin_line(line):
    parts = line.split()
    if len(parts) == 2 and parts[0] == "BEGIN":
        try:
            item_count = int(parts[1])
        except ValueError:
            return None
        return {"format": "samples", "item_count": item_count, "total_samples": item_count}

    if len(parts) == 3 and parts[0] == "BEGIN_HIST":
        try:
            item_count = int(parts[1])
            total_samples = int(parts[2])
        except ValueError:
            return None
        return {"format": "histogram", "item_count": item_count, "total_samples": total_samples}

    return None


def _estimate_transfer_seconds(frame_format, item_count, baudrate, adc_bits, total_samples):
    if frame_format == "histogram":
        max_digits = len(str(total_samples))
        bytes_per_item = max_digits + 2
        frame_bytes = len(f"BEGIN_HIST {item_count} {total_samples}\r\n") + len("END\r\n") + item_count * bytes_per_item
        return frame_bytes * 10.0 / float(baudrate)

    max_digits = len(str((1 << adc_bits) - 1))
    bytes_per_item = max_digits + 2
    frame_bytes = len(f"BEGIN {item_count}\r\n") + len("END\r\n") + item_count * bytes_per_item
    return frame_bytes * 10.0 / float(baudrate)


def _estimate_total_seconds(total_samples, warmup_count, sample_rate, frame_format, item_count, baudrate, adc_bits):
    acquisition_seconds = (total_samples + warmup_count) / float(sample_rate)
    transfer_seconds = _estimate_transfer_seconds(frame_format, item_count, baudrate, adc_bits, total_samples)
    return acquisition_seconds, transfer_seconds, acquisition_seconds + transfer_seconds


def capture_samples(port, baudrate, output_path, timeout, timeout_margin, expected_count, total_samples, sample_rate, warmup_count, adc_bits):
    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    print(f"Abriendo {port} a {baudrate} baud...")
    with serial.Serial(port, baudrate=baudrate, timeout=0.2) as ser:
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        print("Esperando un bloque BEGIN ... END ...")
        expected_acquisition_s, expected_transfer_s, expected_total_s = _estimate_total_seconds(
            total_samples=total_samples,
            warmup_count=warmup_count,
            sample_rate=sample_rate,
            frame_format="histogram",
            item_count=expected_count,
            baudrate=baudrate,
            adc_bits=adc_bits,
        )
        print(
            "Referencia con la configuracion actual: "
            f"captura silenciosa ~{expected_acquisition_s:.2f} s, "
            f"transferencia ~{expected_transfer_s:.2f} s, "
            f"total ~{expected_total_s:.2f} s."
        )

        deadline = None
        block_metadata = None
        captured_lines = []
        capturing = False
        invalid_lines = 0
        invalid_examples = []
        metadata_lines = []
        last_state = None

        while deadline is None or time.monotonic() < deadline:
            raw_line = ser.readline()
            if not raw_line:
                continue

            line = raw_line.decode("ascii", errors="ignore").strip()
            if not line:
                continue

            if not capturing:
                block_metadata = _parse_begin_line(line)
                if block_metadata is None:
                    if line.startswith("#"):
                        metadata_lines.append(line)
                        print(line)
                        if line.startswith("# state="):
                            last_state = line.split("=", 1)[1]
                            if last_state == "capture":
                                print("La etapa de captura no imprime nada hasta terminar la adquisicion.")
                        if line.startswith("# error="):
                            raise RuntimeError(f"El firmware informo {line[2:]}")
                    continue

                item_count = block_metadata["item_count"]
                if expected_count is not None and item_count != expected_count:
                    raise RuntimeError(
                        f"Se recibio BEGIN con {item_count} items, pero se esperaba {expected_count}"
                    )

                effective_total_samples = block_metadata.get("total_samples", total_samples)
                _, transfer_seconds, total_seconds = _estimate_total_seconds(
                    total_samples=effective_total_samples,
                    warmup_count=warmup_count,
                    sample_rate=sample_rate,
                    frame_format=block_metadata["format"],
                    item_count=item_count,
                    baudrate=baudrate,
                    adc_bits=adc_bits,
                )

                capturing = True
                applied_timeout = transfer_seconds + timeout_margin if timeout is None else timeout
                deadline = time.monotonic() + applied_timeout
                if block_metadata["format"] == "histogram":
                    if not any(line.startswith("# total_samples=") for line in metadata_lines):
                        metadata_lines.insert(0, f"# total_samples={effective_total_samples}")
                    if not any(line.startswith("# bins=") for line in metadata_lines):
                        metadata_lines.insert(0, f"# bins={item_count}")
                    if not any(line.startswith("# format=") for line in metadata_lines):
                        metadata_lines.insert(0, "# format=histogram")
                print(f"BEGIN recibido. Esperando {item_count} items...")
                continue

            if line == "END":
                if block_metadata is None:
                    raise RuntimeError("Se recibio END sin un BEGIN valido")
                item_count = block_metadata["item_count"]
                if len(captured_lines) != item_count:
                    details = ""
                    if invalid_lines:
                        details = f" Se descartaron {invalid_lines} lineas no validas"
                        if invalid_examples:
                            joined_examples = ", ".join(repr(example) for example in invalid_examples)
                            details += f". Ejemplos: {joined_examples}"
                    raise RuntimeError(
                        f"Bloque incompleto: se recibieron {len(captured_lines)} items, se esperaban {item_count}."
                        + details
                    )

                file_lines = metadata_lines + captured_lines
                output_path.write_text("\n".join(file_lines) + "\n", encoding="ascii")
                print(f"Captura completada: {len(captured_lines)} items guardados en {output_path}")
                return 0

            if not line.isdigit():
                invalid_lines += 1
                if len(invalid_examples) < 5:
                    invalid_examples.append(line)
                continue

            captured_lines.append(line)

            if block_metadata is not None and len(captured_lines) > block_metadata["item_count"]:
                raise RuntimeError(
                    f"Se recibieron mas items de los esperados ({len(captured_lines)} > {block_metadata['item_count']})"
                )

        raise TimeoutError(
            "Timeout esperando el bloque de muestras por UART. "
            f"Recibidos {len(captured_lines)} items"
            + (f" de {block_metadata['item_count']}" if block_metadata is not None else "")
            + (f". Ultimo estado del firmware: {last_state}" if last_state is not None else "")
            + "."
        )


def main():
    parser = _build_parser()
    args = parser.parse_args()

    if args.list_ports:
        raise SystemExit(_list_ports())

    if not args.port:
        parser.error("Debes indicar el puerto serie, por ejemplo COM5")

    try:
        raise SystemExit(
            capture_samples(
                port=args.port,
                baudrate=args.baudrate,
                output_path=args.output,
                timeout=args.timeout,
                timeout_margin=args.timeout_margin,
                expected_count=args.expected_count,
                total_samples=args.total_samples,
                sample_rate=args.sample_rate,
                warmup_count=args.warmup_count,
                adc_bits=args.adc_bits,
            )
        )
    except KeyboardInterrupt:
        print("Captura cancelada por el usuario.", file=sys.stderr)
        raise SystemExit(130)
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        raise SystemExit(1)


if __name__ == "__main__":
    main()