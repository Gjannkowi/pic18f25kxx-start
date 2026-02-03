#!/usr/bin/env python3
"""Saleae Logic 2.x helper for capturing and decoding DALI Manchester frames.

Compatible with Saleae Logic 2.x (uses automation API).
For Logic 1.x, use analysis_old_saleae.py instead.

Example usage:
    # Interactive menu
    python analysis_new_saleae.py

    # Direct capture (requires Logic 2 app running with automation enabled)
    python analysis_new_saleae.py capture --seconds 5 --output .
"""
from __future__ import annotations

import argparse
import csv
import json
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Sequence

try:
    from saleae import automation  # type: ignore
    from saleae.automation import LogicDeviceConfiguration, CaptureConfiguration  # type: ignore
except ImportError:
    automation = None


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


def _require_automation():
    if automation is None:
        raise RuntimeError(
            "saleae automation package is not installed.\n"
            "Install via: pip install saleae\n"
            "Also ensure Logic 2 app is running with automation enabled:\n"
            "  Preferences -> Developer -> Enable scripting socket server"
        )


def capture_logic2(args: argparse.Namespace) -> None:
    """Capture using Logic 2.x automation API."""
    _require_automation()

    print("Connecting to Logic 2...")
    manager = automation.Manager.connect()

    # Get connected device
    device_config = LogicDeviceConfiguration(
        enabled_digital_channels=[0, 1],  # DALI RX/TX channels
        digital_sample_rate=args.digital_sample_rate,
    )

    # Configure capture
    capture_config = CaptureConfiguration(
        capture_mode=automation.TimedCaptureMode(duration_seconds=args.seconds)
    )

    print(
        f"Starting capture ({args.seconds}s @ {args.digital_sample_rate}Hz digital)..."
    )

    with manager.start_capture(
        device_configuration=device_config, capture_configuration=capture_config
    ) as capture:
        # Wait for capture to complete
        capture.wait()
        print("Capture finished! Adding Manchester analyzer...")

        # Add Manchester analyzer for channel 0 (RX)
        manchester_rx = capture.add_analyzer(
            "Manchester",
            label="DALI_RX",
            settings={
                "Input Channel": 0,
                "Bit Rate (Bits/s)": 1200,  # DALI bit rate
            },
        )

        # Add Manchester analyzer for channel 1 (TX)
        manchester_tx = capture.add_analyzer(
            "Manchester",
            label="DALI_TX",
            settings={
                "Input Channel": 1,
                "Bit Rate (Bits/s)": 1200,  # DALI bit rate
            },
        )

        # Wait for analyzers to complete
        capture.wait()

        out_dir = Path(args.output)
        out_dir.mkdir(parents=True, exist_ok=True)

        # Export analyzer results
        print("Exporting analyzer data...")
        rx_csv = out_dir / "dali_rx_manchester.csv"
        tx_csv = out_dir / "dali_tx_manchester.csv"

        capture.export_data_table(filepath=str(rx_csv), analyzers=[manchester_rx])
        print(f"RX data exported: {rx_csv}")

        capture.export_data_table(filepath=str(tx_csv), analyzers=[manchester_tx])
        print(f"TX data exported: {tx_csv}")

        print("\n" + "=" * 40)
        print("Capture completed successfully!")
        print(f"Files saved in: {out_dir.absolute()}")
        print("=" * 40)


def _parse_float(value: str) -> float:
    try:
        return float(value)
    except ValueError:
        return float(value.replace(",", "."))


def decode(args: argparse.Namespace) -> None:
    """Decode Manchester CSV export."""
    csv_path = Path(args.csv)
    if not csv_path.exists():
        raise FileNotFoundError(f"File not found: {csv_path}")

    frames: List[Frame] = []
    with csv_path.open(newline="", encoding="utf-8") as csv_file:
        reader = csv.DictReader(csv_file)
        lower_headers = {h.lower(): h for h in reader.fieldnames or []}

        time_key = (
            lower_headers.get("time[s]")
            or lower_headers.get("time [s]")
            or lower_headers.get("start time")
            or lower_headers.get("time")
        )
        data_key = lower_headers.get("data") or lower_headers.get("value")
        if not time_key or not data_key:
            raise ValueError(
                "CSV must contain 'Time' and 'Data' columns - export from Saleae analyzer first."
            )

        source_key = lower_headers.get("channel") or lower_headers.get("source")

        for row in reader:
            raw_value = row[data_key].strip()
            if not raw_value:
                continue

            if raw_value.startswith("0x"):
                value_int = int(raw_value, 16)
            else:
                try:
                    value_int = int(raw_value)
                except ValueError:
                    continue

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

    print(f"\nDecoded {len(frames)} frames from {csv_path}:")
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
        print(f"\nJSON summary saved: {json_path}")


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Saleae Logic 2.x capture + DALI Manchester decoder helper."
    )
    sub = parser.add_subparsers(dest="command", required=True)

    capture_parser = sub.add_parser(
        "capture", help="Capture directly from Saleae Logic 2"
    )
    capture_parser.add_argument(
        "--seconds",
        type=float,
        default=5.0,
        help="Capture length in seconds (default: 5.0)",
    )
    capture_parser.add_argument(
        "--digital-sample-rate",
        type=int,
        default=24_000_000,
        help="Digital sample rate (Hz, default: 24MHz)",
    )
    capture_parser.add_argument(
        "--output", type=str, default=".", help="Directory for exported data"
    )
    capture_parser.set_defaults(func=capture_logic2)

    decode_parser = sub.add_parser(
        "decode", help="Decode a Saleae Manchester CSV export"
    )
    decode_parser.add_argument("--csv", required=True, help="Path to the exported CSV")
    decode_parser.add_argument(
        "--label",
        default="DALI",
        help="Fallback label if CSV does not include a Source/Channel column",
    )
    decode_parser.add_argument("--json", help="Optional path to dump JSON summary")
    decode_parser.set_defaults(func=decode)

    return parser


def interactive_menu() -> int:
    """Interactive menu for users who run the script without arguments."""
    while True:
        print("\n" + "=" * 40)
        print("=== Saleae Logic 2.x DALI Analyzer ===")
        print("=" * 40)
        print("1. Quick DALI Manchester Capture (5s)")
        print("2. Quick DALI Decode (ostatni plik)")
        print("3. Custom Capture")
        print("4. Decode CSV file")
        print("5. Exit")
        print("=" * 40)

        choice = input("\nWybierz opcje (1-5): ").strip()

        if choice == "1":
            # Quick DALI Manchester Capture
            print("\n--- Quick DALI Manchester Capture ---")
            print("Ustawienia:")
            print("  - Czas: 5 sekund")
            print("  - Kanaly: 0 i 1 (RX/TX)")
            print("  - Sample rate: 24 MHz")
            print("  - Manchester analyzer: automatycznie dodany")

            script_dir = Path(__file__).parent
            print(f"  - Folder wyjsciowy: {script_dir}")

            confirm = input("\nRozpocznac capture? (T/n): ").strip().lower()
            if confirm and confirm not in ["t", "tak", "y", "yes", ""]:
                print("Anulowano.")
                input("\nNacisnij Enter aby wrocic do menu...")
                continue

            args_list = [
                "capture",
                "--seconds",
                "5.0",
                "--digital-sample-rate",
                "24000000",
                "--output",
                str(script_dir),
            ]

            parser = build_arg_parser()
            args = parser.parse_args(args_list)
            try:
                args.func(args)
            except Exception as exc:
                print(f"\nBlad: {exc}")
                print("\nUpewnij sie ze:")
                print("  1. Aplikacja Logic 2 jest uruchomiona")
                print(
                    "  2. Preferences -> Developer -> Enable scripting socket server jest wlaczone"
                )
                print("  3. Urzadzenie Saleae jest podlaczone")

            input("\nNacisnij Enter aby wrocic do menu...")

        elif choice == "2":
            # Quick DALI Decode - find and decode latest CSV
            print("\n--- Quick DALI Decode ---")
            script_dir = Path(__file__).parent

            # Find all CSV files in script directory
            csv_files = [
                f for f in script_dir.glob("*.csv") if f.name.startswith("dali_")
            ]

            if not csv_files:
                print(
                    f"Blad: Brak plikow CSV zaczynajacych sie od 'dali_' w {script_dir}"
                )
                print("Wykonaj najpierw Quick DALI Capture (opcja 1).")
                input("\nNacisnij Enter aby wrocic do menu...")
                continue

            # Sort by modification time and get the latest
            latest_csv = max(csv_files, key=lambda p: p.stat().st_mtime)

            print(f"Znaleziono plik: {latest_csv.name}")
            print(f"Sciezka: {latest_csv}")

            # Create JSON output path
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
            except Exception as exc:
                print(f"\nBlad: {exc}")

            input("\nNacisnij Enter aby wrocic do menu...")

        elif choice == "3":
            # Custom capture
            print("\n--- Custom Capture Configuration ---")
            seconds = input("Czas capture w sekundach [5.0]: ").strip() or "5.0"
            sample_rate = (
                input("Czestotliwosc probkowania (Hz) [24000000]: ").strip()
                or "24000000"
            )
            output = input("Folder wyjsciowy [.]: ").strip() or "."

            args_list = [
                "capture",
                "--seconds",
                seconds,
                "--digital-sample-rate",
                sample_rate,
                "--output",
                output,
            ]

            parser = build_arg_parser()
            args = parser.parse_args(args_list)
            try:
                args.func(args)
            except Exception as exc:
                print(f"\nBlad: {exc}")

            input("\nNacisnij Enter aby wrocic do menu...")

        elif choice == "4":
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
            label = input("Etykieta kanalu [DALI]: ").strip() or "DALI"

            args_list = ["decode", "--csv", csv_path, "--label", label]
            if json_path:
                args_list.extend(["--json", json_path])

            parser = build_arg_parser()
            args = parser.parse_args(args_list)
            try:
                args.func(args)
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
    except Exception as exc:
        parser.error(str(exc))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
