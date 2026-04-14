import argparse
import sys
import time
from pathlib import Path

import serial
from serial.tools import list_ports


def _build_parser():
    parser = argparse.ArgumentParser(
        description="Captura 4096 pares 'codigo_adc ocurrencias' enviados por UART por el firmware del Pico y los guarda en un TXT."
    )
    parser.add_argument("port", nargs="?", help="Puerto serie, por ejemplo COM5")
    parser.add_argument(
        "--baudrate",
        type=int,
        default=115200,
        help="Baudrate UART del firmware (default: 115200)",
    )
    parser.add_argument(
        "--output",
        default="samples.txt",
        help="Archivo de salida (default: samples.txt)",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=60.0,
        help="Tiempo maximo para recibir el bloque completo, en segundos (default: 60)",
    )
    parser.add_argument(
        "--expected-count",
        type=int,
        default=4096,
        help="Cantidad esperada de pares codigo-conteo dentro del bloque (default: 4096)",
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


def _normalize_payload_line(line):
    normalized = line.replace(",", " ").replace(";", " ").replace("-", " ")
    parts = normalized.split()
    if len(parts) != 2:
        return None
    if not all(part.isdigit() for part in parts):
        return None
    return " ".join(parts)


def capture_samples(port, baudrate, output_path, timeout, expected_count):
    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    print(f"Abriendo {port} a {baudrate} baud...")
    with serial.Serial(port, baudrate=baudrate, timeout=0.2) as ser:
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        print("Esperando un bloque BEGIN ... END ...")

        deadline = time.monotonic() + timeout
        block_count = None
        records = []
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
                print(f"BEGIN recibido. Esperando {block_count} pares codigo-conteo...")
                continue

            if line == "END":
                if block_count is None:
                    raise RuntimeError("Se recibio END sin un BEGIN valido")
                if len(records) != block_count:
                    raise RuntimeError(
                        f"Bloque incompleto: se recibieron {len(records)} lineas, se esperaban {block_count}"
                    )

                output_path.write_text("\n".join(records) + "\n", encoding="ascii")
                print(f"Captura completada: {len(records)} lineas guardadas en {output_path}")
                return 0

            normalized_line = _normalize_payload_line(line)
            if normalized_line is None:
                continue

            records.append(normalized_line)

            if block_count is not None and len(records) > block_count:
                raise RuntimeError(
                    f"Se recibieron mas lineas de las esperadas ({len(records)} > {block_count})"
                )

        raise TimeoutError("Timeout esperando el bloque de histograma por UART")


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