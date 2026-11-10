#!/usr/bin/env python3
"""
@file gsm_monitor.py
@brief Collect GSM radio measurements from the OsmocomBB VTY.

Parses MON events and stores timestamped measurements in CSV.
"""

import csv
import re
import telnetlib
from datetime import datetime, timezone
from pathlib import Path

HOST = "127.0.0.1"
PORT = 4247
OUTPUT = Path("gsm_measurements.csv")

PATTERN = re.compile(
    r"% MON:\s*f=(\d+)\s+"
    r"lev=\s*(-?\d+)\s+"
    r"snr=\s*(-?\d+)\s+"
    r"ber=\s*(\d+)\s+"
    r"CGI=([\d-]+)"
)

FIELDS = [
    "timestamp_utc",
    "arfcn",
    "rx_level_dbm",
    "snr_raw",
    "ber_raw",
    "cgi",
]

def main():
    print("Connecting to OsmocomBB VTY...")

    with telnetlib.Telnet(HOST, PORT, timeout=5) as tn:
        tn.read_until(b"OsmocomBB(mobile)>", timeout=5)

        tn.write(b"enable\n")
        tn.read_until(b"OsmocomBB(mobile)#", timeout=5)

        tn.write(b"monitor network 1\n")

        print("GSM monitoring started.")
        print("Press Ctrl+C to stop. \n")

        new_file = not OUTPUT.exists() or OUTPUT.stat().st_size == 0

        with OUTPUT.open("a", newline="",encoding="utf-8") as f:
            writer = csv.DictWriter(f, fieldnames=FIELDS)

            if new_file:
                writer.writeheader()

            buffer = ""

            while True:
                data = tn.read_some().decode("utf-8", errors="replace")
                buffer += data

                while "\n" in buffer:
                    line, buffer = buffer.split("\n", 1)
                    match = PATTERN.search(line)

                    if not match:
                        continue

                    arfcn, level, snr, ber, cgi = match.groups()

                    row = {
                        "timestamp_utc": datetime.now(
                            timezone.utc
                        ).isoformat(),
                        "arfcn": int(arfcn),
                        "rx_level_dbm": int(level),
                        "snr_raw": int(snr),
                        "ber_raw": int(ber),
                        "cgi": cgi,
                    }

                    writer.writerow(row)
                    f.flush()

                    print(
                        f"ARFCN={arfcn} "
                        f"RX={level} dBm "
                        f"SNR={snr} "
                        f"BER={ber} "
                        f"CGI={cgi}"
                    )




if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nMonitoring stopped.")
    except (OSError, EOFError) as exc:
        print(f"\nVTY connection error: {exc}")
