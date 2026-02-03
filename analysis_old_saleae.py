#!/usr/bin/env python3
"""Saleae helper for capturing and decoding DALI Manchester frames."""
from __future__ import annotations

import argparse
import csv
import json
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List, Optional, Sequence

try:
    from saleae import Saleae  # type: ignore
except ImportError:  # pragma: no cover
    Saleae = None  # type: ignore


@dataclass
class Frame:
    timestamp: float
    value: int
    source: str

    @property
    def ascii(self) -> str:
        return chr(self.value) if 32 <= self.value <= 126 else "."

    def to_dict(self) -> dict:
        return {
            "time": self.timestamp,
            "value": self.value,
            "ascii": self.ascii,
            "source": self.source,
        }


def _require_saleae() -> Saleae:
    if Saleae is None:
        raise RuntimeError(
            "saleae python package is not installed. Install via 'pip install saleae'."
        )
    return Saleae()


def capture(args: argparse.Namespace) -> None:
    logic = _require_saleae()

    logic.set_sample_rate((args.digital_sample_rate, args.analog_sample_rate))
    logic.set_capture_seconds(args.seconds)

    digital_channels = list(args.digital)
    # Note: Channels must be configured manually in the Saleae GUI before capture
    # Some versions don't support setting channels via API

    if args.trigger is not None:
        try:
            logic.set_trigger_one_channel(args.trigger)
        except AttributeError:
            print(
                f"Warning: Trigger not supported in this API version. Configure manually in GUI."
            )

    print(
        f"Starting capture ({args.seconds}s @ {args.digital_sample_rate}Hz digital)..."
    )
    logic.capture_start_and_wait_until_finished()
    print("Capture finished! Exporting data...")

    out_dir = Path(args.output)
    out_dir.mkdir(parents=True, exist_ok=True)
    print(f"Created output directory: {out_dir.absolute()}")

    # Export analyzer data if a Manchester analyzer was configured in the GUI
    analyzers = logic.get_analyzers()
    if analyzers:
        analyzer_path = out_dir / "analyzers"
        analyzer_path.mkdir(exist_ok=True)
        print(f"Found {len(analyzers)} analyzer(s)")
        for analyzer_index, analyzer in enumerate(analyzers):
            target = analyzer_path / f"analyzer_{analyzer_index}_{analyzer.name}.csv"
            print(
                f"Exporting analyzer {analyzer_index} ({analyzer.name}) to {target}..."
            )
            logic.export_analyzer(analyzer_index, str(target))
            print(f"Analyzer export completed -> {target}")
    else:
        print("WARNING: No analyzers configured in Saleae GUI!")
        print("To decode DALI frames, add a Manchester analyzer in Saleae Logic:")
        print("  1. Click '+' to add analyzer")
        print("  2. Select 'Manchester' analyzer")
        print("  3. Configure for your DALI channels")
        print("  4. Run capture again")

    # Try to export raw data as well
    try:
        export_basename = out_dir / "capture"
        print(f"Attempting to export raw data to {export_basename}...")
        logic.export_data2(
            str(out_dir),
            digital_channels=digital_channels,
            analog_channels=[],
            sample_rate=args.digital_sample_rate,
            filename_prefix=str(export_basename),
        )
        print(f"Raw digital samples saved under {out_dir}")
    except Exception as e:
        print(f"Note: Raw data export not available ({e})")


def _parse_float(value: str) -> float:
    try:
        return float(value)
    except ValueError:
        # Saleae may use comma decimal separators depending on locale
        return float(value.replace(",", "."))


def decode(args: argparse.Namespace) -> None:
    csv_path = Path(args.csv)
    if not csv_path.exists():
        raise FileNotFoundError(csv_path)

    frames: List[Frame] = []
    with csv_path.open(newline="", encoding="utf-8") as csv_file:
        reader = csv.DictReader(csv_file)
        lower_headers = {h.lower(): h for h in reader.fieldnames or []}

        time_key = (
            lower_headers.get("time[s]")
            or lower_headers.get("time [s]")
            or lower_headers.get("time")
        )
        data_key = lower_headers.get("data") or lower_headers.get("value")
        if not time_key or not data_key:
            raise ValueError(
                "CSV must contain 'Time [s]' and 'Data' columns - export from Saleae analyzer first."
            )

        source_key = lower_headers.get("channel") or lower_headers.get("source")

        for row in reader:
            raw_value = row[data_key].strip()
            if not raw_value:
                continue

            if raw_value.startswith("0x"):
                value_int = int(raw_value, 16)
            else:
                value_int = int(raw_value)

            source = row[source_key] if source_key else args.label
            frames.append(
                Frame(
                    timestamp=_parse_float(row[time_key]),
                    value=value_int,
                    source=source,
                )
            )

    if not frames:
        print("No frames decoded - check the CSV export.")
        return

    frames.sort(key=lambda f: f.timestamp)

    print(f"Decoded {len(frames)} frames from {csv_path}:")
    for frame in frames:
        print(
            f"{frame.timestamp:9.6f}s  {frame.source:>8}  0x{frame.value:02X}  {frame.ascii}"
        )

    summary = {
        "file": str(csv_path),
        "count": len(frames),
        "frames": [frame.to_dict() for frame in frames],
    }
    if args.json:
        json_path = Path(args.json)
        json_path.write_text(json.dumps(summary, indent=2), encoding="utf-8")
        print(f"JSON summary -> {json_path}")


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Saleae capture + DALI Manchester decoder helper."
    )
    sub = parser.add_subparsers(dest="command", required=True)

    capture_parser = sub.add_parser(
        "capture", help="Capture directly from Saleae Logic"
    )
    capture_parser.add_argument(
        "--seconds",
        type=float,
        default=1.0,
        help="Capture length in seconds (default: 1.0)",
    )
    capture_parser.add_argument(
        "--digital",
        type=int,
        nargs="+",
        default=[0, 1],
        help="Digital channel indices to enable (default: 0 and 1 for DALI RX/TX)",
    )
    capture_parser.add_argument(
        "--digital-sample-rate",
        type=int,
        default=24_000_000,
        help="Digital sample rate (Hz, default: 24MHz for Saleae 8ch)",
    )
    capture_parser.add_argument(
        "--analog-sample-rate", type=int, default=0, help="Analog sample rate (Hz)"
    )
    capture_parser.add_argument(
        "--trigger", type=int, default=None, help="Optional trigger digital channel"
    )
    capture_parser.add_argument(
        "--output", type=str, default="captures", help="Directory for exported data"
    )
    capture_parser.set_defaults(func=capture)

    decode_parser = sub.add_parser(
        "decode", help="Decode a Saleae Manchester CSV export"
    )
    decode_parser.add_argument("--csv", required=True, help="Path to the exported CSV")
    decode_parser.add_argument(
        "--label",
        default="Channel",
        help="Fallback label used if CSV does not include a Source/Channel column",
    )
    decode_parser.add_argument("--json", help="Optional path to dump JSON summary")
    decode_parser.set_defaults(func=decode)

    return parser


def interactive_menu() -> int:
    """Interactive menu for users who run the script without arguments."""
    while True:
        print("\n" + "=" * 40)
        print("=== Saleae DALI Analyzer ===")
        print("=" * 40)
        print("1. Capture from Saleae Logic")
        print("2. Decode CSV file")
        print("3. Quick DALI Manchester Capture (5s)")
        print("4. Quick DALI Decode (ostatni plik)")
        print("5. Exit")
        print("=" * 40)

        choice = input("\nWybierz opcje (1-5): ").strip()

        if choice == "1":
            # Capture mode
            print("\n--- Capture Configuration ---")
            seconds = input("Czas capture w sekundach [1.0]: ").strip() or "1.0"
            channels = (
                input("Kanaly cyfrowe (rozdzielone spacja) [0 1]: ").strip() or "0 1"
            )
            sample_rate = (
                input("Czestotliwosc probkowania (Hz) [24000000]: ").strip()
                or "24000000"
            )
            output = input("Folder wyjsciowy [captures]: ").strip() or "captures"
            trigger = input("Kanal trigger (Enter aby pominac): ").strip()

            args_list = [
                "capture",
                "--seconds",
                seconds,
                "--digital",
                *channels.split(),
                "--digital-sample-rate",
                sample_rate,
                "--output",
                output,
            ]
            if trigger:
                args_list.extend(["--trigger", trigger])

            parser = build_arg_parser()
            args = parser.parse_args(args_list)
            try:
                args.func(args)
                print("\n" + "=" * 40)
                print("Capture zakonczone pomyslnie!")
                print("=" * 40)
            except Exception as exc:
                print(f"\nBlad: {exc}")

            input("\nNacisnij Enter aby wrocic do menu...")

        elif choice == "2":
            # Decode mode
            print("\n--- Decode Configuration ---")
            csv_path = input("Sciezka do pliku CSV: ").strip()
            if not csv_path:
                print("Blad: Musisz podac sciezke do pliku CSV")
                input("\nNacisnij Enter aby wrocic do menu...")
                continue

            json_path = input(
                "Sciezka do pliku JSON wyjsciowego (Enter aby pominac): "
            ).strip()
            label = input("Etykieta kanalu [Channel]: ").strip() or "Channel"

            args_list = ["decode", "--csv", csv_path, "--label", label]
            if json_path:
                args_list.extend(["--json", json_path])

            parser = build_arg_parser()
            args = parser.parse_args(args_list)
            try:
                args.func(args)
                print("\n" + "=" * 40)
                print("Dekodowanie zakonczone pomyslnie!")
                print("=" * 40)
            except Exception as exc:
                print(f"\nBlad: {exc}")

            input("\nNacisnij Enter aby wrocic do menu...")

        elif choice == "3":
            # Quick DALI Manchester Capture with preset settings
            print("\n--- Quick DALI Manchester Capture ---")
            print("Ustawienia:")
            print("  - Czas: 5 sekund")
            print("  - Kanaly: 0 i 1 (RX/TX)")
            print("  - Sample rate: 24 MHz")

            script_dir = Path(__file__).parent
            output_dir = script_dir

            print(f"  - Folder wyjsciowy: {output_dir}")

            confirm = input("\nRozpocznac capture? (T/n): ").strip().lower()
            if confirm and confirm not in ["t", "tak", "y", "yes", ""]:
                print("Anulowano.")
                input("\nNacisnij Enter aby wrocic do menu...")
                continue

            args_list = [
                "capture",
                "--seconds",
                "5.0",
                "--digital",
                "0",
                "1",
                "--digital-sample-rate",
                "24000000",
                "--output",
                str(output_dir),
            ]

            parser = build_arg_parser()
            args = parser.parse_args(args_list)
            try:
                args.func(args)
                print("\n" + "=" * 40)
                print("Capture zakonczone pomyslnie!")
                print(f"Pliki zapisane w: {output_dir}")
                print("=" * 40)
            except Exception as exc:
                print(f"\nBlad: {exc}")

            input("\nNacisnij Enter aby wrocic do menu...")

        elif choice == "4":
            # Quick DALI Decode - find and decode latest CSV
            print("\n--- Quick DALI Decode ---")
            script_dir = Path(__file__).parent

            # Find all CSV files in script directory and subdirectories
            csv_files = list(script_dir.rglob("*.csv"))

            if not csv_files:
                print(f"Blad: Brak plikow CSV w {script_dir}")
                print("Wykonaj najpierw Quick DALI Capture (opcja 3).")
                input("\nNacisnij Enter aby wrocic do menu...")
                continue

            # Sort by modification time and get the latest
            latest_csv = max(csv_files, key=lambda p: p.stat().st_mtime)

            print(f"Znaleziono plik: {latest_csv.name}")
            print(f"Sciezka: {latest_csv}")

            # Create JSON output path in the same location
            json_output = latest_csv.with_suffix(".json")

            args_list = [
                "decode",
                "--csv",
                str(latest_csv),
                "--label",
                "DALI",
                "--json",
                str(json_output),
            ]

            parser = build_arg_parser()
            args = parser.parse_args(args_list)
            try:
                args.func(args)
                print("\n" + "=" * 40)
                print("Dekodowanie zakonczone pomyslnie!")
                print(f"JSON zapisany: {json_output}")
                print("=" * 40)
            except Exception as exc:
                print(f"\nBlad: {exc}")

            input("\nNacisnij Enter aby wrocic do menu...")

        elif choice == "5":
            print("\nDo widzenia!")
            return 0
        else:
            print("\nNieprawidlowy wybor! Wybierz 1, 2, 3, 4 lub 5.")
            input("\nNacisnij Enter aby kontynuowac...")

    return 0


def main(argv: Optional[Sequence[str]] = None) -> int:
    # If no arguments provided and running interactively, show menu
    if argv is None and len(sys.argv) == 1:
        return interactive_menu()

    parser = build_arg_parser()
    args = parser.parse_args(argv)
    try:
        args.func(args)
    except Exception as exc:  # pragma: no cover
        parser.error(str(exc))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
