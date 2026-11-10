# GSM Gate Controller

**An experimental GSM modem integration and verification project using OsmocomBB, a Motorola C121, and a Raspberry Pi.**

The eventual application is to use authorized incoming-call events to request button presses on an existing two-button gate remote. The project currently focuses on reproducible firmware builds, GSM stack integration, passive cell measurements, and automated verification. The gate-control feature is **not yet implemented or validated**.

## Project goals

- Build and run OsmocomBB Layer 1 firmware on the Motorola C121 (Compal E88).
- Run the OsmocomBB `osmocon` loader and `mobile` Layer 2/3 application on a Raspberry Pi.
- Observe GSM cell-selection and radio measurements through the local VTY interface.
- Develop a Python test harness for event parsing, timestamped measurements, and regression testing.
- Later investigate incoming-call event handling and a two-channel isolated interface to the original rolling-code remote.

## Hardware

| Item | Role | Status |
| --- | --- | --- |
| Motorola **C121** | GSM baseband target (Compal E88) | Firmware running in RAM |
| Raspberry Pi Zero W / Zero 2 W | Linux host for `osmocon`, `mobile`, and Python | Debian 12 ARM64 host used; exact Pi variant not yet recorded |
| Silicon Labs **CP2102** USB-to-UART adapter (`10c4:ea60`) | Serial connection to phone | Working via `/dev/ttyUSB0` |
| Original two-button gate remote | Existing radio transmitter; no code extraction planned | Not connected |
| 5 V two-channel relay module | Future electrically isolated button actuation | Not connected or verified |
| Independent Motorola charger; suitable Raspberry Pi 5 V supply | Power | Wiring and supply tests still to be documented |

The Raspberry Pi onboard UART `/dev/serial0` resolved to `/dev/ttyAMA0`, but the **successful** loader and firmware integration used the CP2102 USB adapter, not the onboard UART. Exact adapter wiring and voltage-level measurements have not yet been recorded.

## Architecture

```text
Motorola C121 (Compal E88)
  OsmocomBB Layer 1 firmware (RAM)
             |
          UART / CP2102
             |
Raspberry Pi (Debian 12 ARM64)
  osmocon -> /tmp/osmocom_l2 -> mobile (GSM Layer 2/3)
                                     |
                              VTY 127.0.0.1:4247
                                     |
                             Python verification
                                     |
                        CSV logs / automated tests

Future integration (not implemented):
  authenticated call event -> controller state machine
                           -> GPIO -> isolated relay contacts
                           -> original remote button contacts
```

## Verified progress

- **PASS:** OsmocomBB host programs built; `osmocon -h` works and `mobile` starts.
- **PASS:** Motorola C121 runs the RAM-resident Layer 1 firmware (ongoing `BAT-ADC` output observed).
- **PASS:** Layer 1/2/3 integration via `/tmp/osmocom_l2`; VTY reports `MS '1' is up`.
- **PASS:** Passive GSM cell discovery **without a SIM**; serving ARFCN XX, CGI `262-01-XXXXX-XXXX`.
- **PASS:** Live radio-monitor output through the VTY interface.
- **NOT RUN:** Python CSV-monitor execution, SIM registration, incoming calls, caller verification, relay actuation, and physical gate-control tests.

A running monitor is not equivalent to a registered GSM subscriber. The `mobile` VTY reported `IMSI detached` and `No SIM present` during these tests.

## Documentation

- [Build OsmocomBB on Raspberry Pi](docs/build-raspberry-pi.md): dependencies, pinned source commits, toolchain and troubleshooting.
- [Verification and integration](docs/verification.md): test cases, observed logs, evidence, and open work.
- `docs/hardware.md` (planned): wiring, serial voltage levels, power measurements, and relay interface.

## Reproduce the software setup

Follow the [build guide](docs/build-raspberry-pi.md). In particular, the recorded Debian 12 ARM64 build needed **source-built** `libosmocore` and `libosmo-gprs`, rather than relying solely on the distribution's `libosmocore-dev` package.

After the Motorola Layer 1 firmware is loaded, `mobile` can be started using the installed sample configuration and its VTY inspected locally. Specific commands and caveats are in the build guide.

## Development roadmap

1. Capture the exact successful loader invocation, physical cable wiring, and Pi board model.
2. Validate the Python GSM measurement collector with real VTY output and synthetic parser fixtures.
3. Add automated unit, integration, and fault-injection tests.
4. Experiment with SIM/network and incoming-call event handling on a compatible, authorized network setup.
5. Prototype isolated output control against a dummy load before any gate hardware integration.

### GSM Measurement Monitor

The Python monitor connects to the OsmocomBB VTY interface and collects timestamped GSM radio measurements, including ARFCN, received signal level, and raw SNR/BER values.

The measurements can be exported to CSV for further analysis and visualization.

See [Verification and Integration](docs/verification.md) for test results and limitations.

## Contributions

Issues and pull requests are welcome, especially for reproducible ARM64 builds, version compatibility, GSM VTY parsing, test fixtures, and documentation. Include the relevant commit hashes, compiler versions, and logs (redact personal identifiers).

## Project status

**Prototype / research.** This repository documents observed integration results and planned work; it does not claim a completed or production-ready gate controller.

OsmocomBB and the Osmocom libraries are third-party projects with their own upstream licenses. Consult each upstream repository for its applicable license. This repository's own license has not yet been selected.
