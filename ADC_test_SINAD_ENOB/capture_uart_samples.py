import argparse
import sys
import time
from pathlib import Path

import serial
from serial.tools import list_ports


def _build_parser():
    parser = argparse.ArgumentParser(
        description="Captura un bloque de muestras ADC enviadas por UART y lo guarda en un TXT."
    )
    parser.add_argument("port", nargs="?", help="Puerto serie, por ejemplo COM5")
    parser.add_argument(
        "--baudrate",
        type=int,
        default=115200,
        help="Baudrate UART (default: 115200)",
    )
    parser.add_argument(
        "--output",
        default="samples.txt",
        help="Archivo de salida (default: samples.txt)",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=30.0,
        help="Tiempo maximo para recibir el bloque completo, en segundos (default: 30)",
    )
    parser.add_argument(
        "--expected-count",
        type=int,
        default=None,
        help="Cantidad esperada de muestras. Si se omite, se usa el valor enviado en BEGIN.",
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
    if len(parts) != 2 or parts[0] != "BEGIN":
        return None
    try:
        return int(parts[1])
    except ValueError:
        return None


def capture_samples(port, baudrate, output_path, timeout, expected_count):
    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    print(f"Abriendo {port} a {baudrate} baud...")
    with serial.Serial(port, baudrate=baudrate, timeout=0.2) as ser:
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        print("Esperando encabezado BEGIN...")

        deadline = time.monotonic() + timeout
        block_count = None
        samples = []
        capturing = False

        while time.monotonic() < deadline:
            raw_line = ser.readline()
            if not raw_line:
                continue

            line = raw_line.decode("ascii", errors="ignore").strip()
            if not line:
                continue

            if not capturing:
                block_count = _parse_begin_line(line)
                if block_count is None:
                    continue

                if expected_count is not None and block_count != expected_count:
                    raise RuntimeError(
                        f"Se recibio BEGIN {block_count}, pero se esperaba {expected_count}"
                    )

                capturing = True
                print(f"BEGIN recibido. Esperando {block_count} muestras...")
                continue

            if line == "END":
                if block_count is None:
                    raise RuntimeError("Se recibio END sin un BEGIN valido")
                if len(samples) != block_count:
                    raise RuntimeError(
                        f"Bloque incompleto: se recibieron {len(samples)} muestras, se esperaban {block_count}"
                    )

                output_path.write_text("\n".join(samples) + "\n", encoding="ascii")
                print(f"Captura completada: {len(samples)} muestras guardadas en {output_path}")
                return 0

            if not line.isdigit():
                continue

            samples.append(line)

            if block_count is not None and len(samples) > block_count:
                raise RuntimeError(
                    f"Se recibieron mas muestras de las esperadas ({len(samples)} > {block_count})"
                )

        raise TimeoutError("Timeout esperando el bloque de muestras por UART")


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
                expected_count=args.expected_count,
            )
        )
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        raise SystemExit(1)


if __name__ == "__main__":
    main()