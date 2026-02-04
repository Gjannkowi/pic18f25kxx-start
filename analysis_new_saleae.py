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
    from saleae import automation
    from saleae.automation import (
        Manager,
        CaptureConfiguration,
        LogicDeviceConfiguration,
    )
    from saleae.automation import TimedCaptureMode

    automation_available = True
except ImportError:
    automation_available = False
    Manager = None
    CaptureConfiguration = None
    LogicDeviceConfiguration = None
    TimedCaptureMode = None


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
    if not automation_available:
        raise RuntimeError(
            "logic2-automation package is not installed.\n"
            "Install via: pip install logic2-automation\n"
            "Also ensure Logic 2 app is running (automation server is enabled by default)."
        )


def capture_logic2(args: argparse.Namespace) -> None:
    """Capture using Logic 2.x automation API."""
    _require_automation()

    print("Connecting to Logic 2...")
    try:
        manager = Manager.connect(port=10430, connect_timeout_seconds=10)
    except Exception as e:
        raise RuntimeError(
            f"Nie mozna polaczyc sie z Logic 2: {e}\n"
            "Upewnij sie ze:\n"
            "  1. Aplikacja Logic 2 jest uruchomiona\n"
            "  2. Edit -> Settings -> Automation -> Enable automation server jest wlaczone\n"
            "  3. Port 10430 jest dostepny"
        )

    # Get connected device
    device_config = LogicDeviceConfiguration(
        enabled_digital_channels=[0, 1],  # DALI RX/TX channels
        digital_sample_rate=args.digital_sample_rate,
    )

    # Configure capture
    capture_config = CaptureConfiguration(
        capture_mode=TimedCaptureMode(duration_seconds=args.seconds)
    )

    print(
        f"Starting capture ({args.seconds}s @ {args.digital_sample_rate}Hz digital)..."
    )

    try:
        capture = manager.start_capture(
            device_configuration=device_config, capture_configuration=capture_config
        )

        # Wait for capture to complete
        print("Waiting for capture to complete...")
        capture.wait()
        print("Capture finished! Adding Manchester analyzer...")

        # Add Manchester analyzer for channel 0 (RX)
        # RX jest podciagniety do zasilania i kluczowany do zera (pull-up, inwersja)
        manchester_rx = capture.add_analyzer(
            "Manchester",
            label="DALI_RX",
            settings={
                "Manchester": 0,
                "Bit Rate (Bits/s)": 1200,  # DALI bit rate
                "Edge Polarity": "negative edge is binary zero",  # RX pull-up
            },
        )

        # Add Manchester analyzer for channel 1 (TX)
        # TX jest 0 i podnosi sie do zasilania (push-pull, bez inwersji)
        manchester_tx = capture.add_analyzer(
            "Manchester",
            label="DALI_TX",
            settings={
                "Manchester": 1,
                "Bit Rate (Bits/s)": 1200,  # DALI bit rate
                "Edge Polarity": "negative edge is binary one",  # TX push-pull
            },
        )

        # Wait for analyzers to complete
        print("Processing analyzers...")
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

        # Close capture
        capture.close()

        print("\n" + "=" * 40)
        print("Capture completed successfully!")
        print(f"Files saved in: {out_dir.absolute()}")
        print("=" * 40)

    except Exception as e:
        print(f"\nBlad podczas capture: {e}")
        raise


def _parse_float(value: str) -> float:
    try:
        return float(value)
    except ValueError:
        return float(value.replace(",", "."))


def _decode_csv_file(csv_path: Path, label: str) -> List[Frame]:
    """Decode CSV file and return list of frames."""
    frames: List[Frame] = []

    with csv_path.open(newline="", encoding="utf-8") as csv_file:
        reader = csv.DictReader(csv_file)
        lower_headers = {h.lower(): h for h in reader.fieldnames or []}

        time_key = (
            lower_headers.get("start_time")
            or lower_headers.get("time[s]")
            or lower_headers.get("time [s]")
            or lower_headers.get("start time")
            or lower_headers.get("time")
        )
        data_key = lower_headers.get("data") or lower_headers.get("value")
        if not time_key or not data_key:
            return frames  # Return empty list if columns not found

        source_key = (
            lower_headers.get("name")
            or lower_headers.get("channel")
            or lower_headers.get("source")
        )

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

            source = row[source_key] if source_key else label
            frames.append(
                Frame(
                    timestamp=_parse_float(row[time_key]),
                    value=value_int,
                    source=source,
                )
            )

    return frames


def decode(args: argparse.Namespace) -> None:
    """Decode Manchester CSV export."""
    csv_path = Path(args.csv)
    if not csv_path.exists():
        raise FileNotFoundError(f"File not found: {csv_path}")

    frames = _decode_csv_file(csv_path, args.label)

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
        default=12.0,
        help="Capture length in seconds (default: 12.0)",
    )
    capture_parser.add_argument(
        "--digital-sample-rate",
        type=int,
        default=12_000_000,
        help="Digital sample rate (Hz, default: 12MHz)",
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
        print("1. Quick DALI Manchester Capture (20s)")
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
            print("  - Czas: 20 sekund")
            print("  - Kanaly: 0 i 1 (RX/TX)")
            print("  - Sample rate: 2 MHz")
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
                "12.0",
                "--digital-sample-rate",
                "12000000",
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
            # Quick DALI Decode - find and decode latest CSV files (RX and TX)
            print("\n--- Quick DALI Decode ---")
            script_dir = Path(__file__).parent

            # Find RX and TX CSV files
            rx_csv = script_dir / "dali_rx_manchester.csv"
            tx_csv = script_dir / "dali_tx_manchester.csv"

            all_frames: List[Frame] = []

            # Decode RX file if exists
            if rx_csv.exists():
                print(f"Dekodowanie RX: {rx_csv.name}")
                parser = build_arg_parser()
                args = parser.parse_args(
                    ["decode", "--csv", str(rx_csv), "--label", "DALI_RX"]
                )
                try:
                    # Read frames from RX
                    frames_rx = _decode_csv_file(rx_csv, "DALI_RX")
                    all_frames.extend(frames_rx)
                    print(f"  Znaleziono {len(frames_rx)} ramek RX")
                except Exception as exc:
                    print(f"  Blad RX: {exc}")
            else:
                print(f"Plik RX nie istnieje: {rx_csv}")

            # Decode TX file if exists
            if tx_csv.exists():
                print(f"Dekodowanie TX: {tx_csv.name}")
                try:
                    frames_tx = _decode_csv_file(tx_csv, "DALI_TX")
                    all_frames.extend(frames_tx)
                    print(f"  Znaleziono {len(frames_tx)} ramek TX")
                except Exception as exc:
                    print(f"  Blad TX: {exc}")
            else:
                print(f"Plik TX nie istnieje: {tx_csv}")

            if not all_frames:
                print("\nBrak ramek do wyswietlenia.")
                input("\nNacisnij Enter aby wrocic do menu...")
                continue

            # Sort all frames by timestamp
            all_frames.sort(key=lambda f: f.timestamp)

            print(f"\n{'='*60}")
            print(f"Zdekodowano lacznie {len(all_frames)} ramek:")
            print(f"{'='*60}")
            for frame in all_frames:
                print(
                    f"{frame.timestamp:9.6f}s  {frame.source:>10}  0x{frame.value:02X}  {frame.ascii}"
                )

            # Save to JSON
            json_output = script_dir / "dali_combined.json"
            summary = {
                "files": [str(rx_csv), str(tx_csv)],
                "frame_count": len(all_frames),
                "frames": [f.to_dict() for f in all_frames],
            }
            with json_output.open("w", encoding="utf-8") as json_file:
                json.dump(summary, json_file, indent=2)
            print(f"\nJSON zapisany: {json_output}")

            input("\nNacisnij Enter aby wrocic do menu...")

        elif choice == "3":
            # Custom capture
            print("\n--- Custom Capture Configuration ---")
            seconds = input("Czas capture w sekundach [12.0]: ").strip() or "12.0"
            sample_rate = (
                input("Czestotliwosc probkowania (Hz) [12000000]: ").strip()
                or "12000000"
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
