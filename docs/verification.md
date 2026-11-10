# Verification and Integration

This document records **what has been tested and observed**, rather than repeating the setup instructions. Build dependencies, exact source revisions, and compiler versions are recorded in [Building OsmocomBB on Raspberry Pi](build-raspberry-pi.md).

## Test environment and provenance

| Item | Recorded value |
| --- | --- |
| Target phone | **Motorola C121**, Compal E88 family |
| Host system | Raspberry Pi, Debian GNU/Linux 12 (Bookworm), ARM64 (`aarch64`) |
| Serial transport | Silicon Labs **CP2102** (`10c4:ea60`), `/dev/ttyUSB0` |
| Unused successful-path alternative | Pi GPIO UART `/dev/serial0` -> `/dev/ttyAMA0`; unsuccessful in the initial trials |
| Phone firmware | Locally built OsmocomBB `layer1.compalram.bin`, RAM-resident, ~62 KiB |
| Host software | `osmocon` and `mobile` |
| VTY endpoint | `127.0.0.1:4247` |
| SIM during captured tests | **Not inserted** |
| Native GCC | `12.2.0` (Debian `12.2.0-14+deb12u1`) |
| Cross-compiler | `arm-none-eabi-gcc 12.2.1 20221205` (Debian package `15:12.2.rel1-1`) |

### Git commits

| Source | Commit |
| --- | --- |
| OsmocomBB | `168b8c5fe49e84a8a8142e64519085b363b4982a` |
| libosmocore | `1b72c0288c3f6fcec916cc439623af1c1c4b5d4c` |
| libosmo-gprs | `6763a0c6aeab0424e2e41a2d500df38f47846234` |

These exact revisions were recorded from the Raspberry Pi checkout on 2026-11-05. The specific Raspberry Pi board variant, measured adapter voltage levels, full UART wiring, and exact successful firmware-loader retry sequence remain to be captured.

## Architecture under test

```text
Motorola C121 (Layer 1 firmware in RAM)
               |
          CP2102 USB-UART
               |
Raspberry Pi: osmocon -- /tmp/osmocom_l2 -- mobile (Layer 2/3)
                                              |
                                     VTY localhost:4247
                                              |
                                   Live monitor output
                                              |
                               Python CSV monitor (planned)
```

## Recorded test matrix

`PASS` denotes an observation supported by the user's command output, not certification of the full gate-control system.

| ID | Verification objective | Observed evidence | Status |
| --- | --- | --- | --- |
| GSM-BLD-001 | Build host applications | `osmocon -h` returned usage; `layer23` build reported successful; `mobile` launched | **PASS** |
| GSM-FW-001 | Boot Layer 1 firmware on C121 | Repeated `BAT-ADC` and charging telemetry after loader attempts | **PASS (execution evidenced)** |
| GSM-INT-001 | Connect firmware and host stack | `/tmp/osmocom_l2` socket existed; VTY reported `MS '1' is up` | **PASS** |
| GSM-RX-001 | Camp on a detectable GSM cell without a SIM | `C7 camped on any cell`, ARFCN XX, CGI `262-01-XXXXX-XXXX` | **PASS** |
| GSM-RX-002 | Enumerate observable cells | `show cell 1` listed ARFCNs | **PASS** |
| GSM-MON-001 | Obtain live radio measurements | `monitor network 1` emitted serving and neighbor measurements; selected samples `lev=-57`, `snr=1..9`, `ber=0` | **PASS** |
| GSM-MON-002 | Persist VTY measurements to CSV using Python | Example script prepared; no completed execution/log provided | **NOT RUN** |
| GSM-MON-003 | Test parser against malformed, partial, interleaved and disconnected VTY output | No automated results captured | **NOT RUN** |
| GSM-SIM-001 | Read SIM and register with GSM network | No SIM inserted; `No SIM present` and `IMSI detached` | **NOT RUN** |
| GSM-CALL-001 | Detect incoming GSM calls | Not attempted | **NOT RUN** |
| GSM-IO-001 | Verify isolated output pulses against a dummy load | Not attempted | **NOT RUN** |
| GSM-GATE-001 | Integrate actual remote button contacts and gate controller | Not attempted | **NOT RUN** |

## Captured evidence

### Mobile Station status

```text
MS '1' is up, service is limited (IMSI detached)
automatic network selection state: A6 no SIM inserted
cell selection state: C7 camped on any cell
                      ARFCN=XX CGI=262-01-XXXXX-XXXX
radio resource layer state: idle
mobility management layer state: MM idle, no IMSI
```

### Cell discovery

| ARFCN | RX level (dBm) |
| ---: | ---: |
| XX | -80 |
| XX | -94 |
| XX | -90 |
| XX | -75 |
| XX | -63 |
| XX | -86 |
| XX | -76 |
| XX | -85 |

The values above come from one `show cell 1` output. They are not time-averaged or calibrated RF measurements.

### Live-monitor samples

```text
% MON: f=XX lev=-57 snr= 9 ber=  0 CGI=262-01-XXXXX-XXXX
% MON: cell    ARFCN     LAC    C1  C2  CRH RLA_C bargraph
% MON: serving XX        0xXXXX  50  50      -56  ===========================
% MON: nb  1   XX        0xXXXX  38  38  0   -68  =====================
% MON: f=XX lev=-57 snr= 1 ber=  0 CGI=262-01-XXXXX-XXXX
% MON: f=XX lev=-57 snr= 5 ber=  0 CGI=262-01-XXXXX-XXXX
```

The `snr` and `ber` fields are recorded **as reported by OsmocomBB**; their exact scaling and measurement interpretation have not been verified. A sample `ber=0` does not prove zero errors over a longer interval.

## Integration issues encountered

| Issue | Observation | Outcome / confidence |
| --- | --- | --- |
| Missing `libosmoisdn` | Debian 12 `libosmocore-dev` 1.7.0-3 did not provide the required pkg-config entry for this build | Source-built `libosmocore`; subsequent configure check passed |
| Missing `libosmo-gprs-rlcmac` | Host layer23 configure stopped | Source-built `libosmo-gprs`; layer23 later built successfully |
| Pi onboard UART | `/dev/serial0` produced no usable complete loader exchange | Switched to CP2102 `/dev/ttyUSB0`; underlying cause not isolated |
| Loader NACK | `Received DOWNLOAD NACK` and `FTMTOOL` appeared in early attempts | Later firmware telemetry demonstrated execution; precise successful retry not captured |
| Missing `mobile.cfg` | `mobile` could not parse expected config | Copied actual sample `~/osmocom-bb/doc/examples/mobile/mobile.cfg` |
| Telnet CLI absent | `telnet: command not found` | Installed `telnet`; VTY session worked |
| VTY session disconnected | `Connection closed by foreign host` | User confirmed typing `exit`; expected session termination |

## Verification methodology for next iteration

For each new automated test, record a test ID, target requirement, setup/preconditions, input stimulus, expected behavior, actual output (including saved log or CSV), and a final PASS/FAIL/NOT RUN status. Keep synthetic parser tests separate from results using the real C121 hardware.

Suggested first test group for the Python monitor:

1. **GSM-MON-002:** start `mobile` and the CSV collector; check that a real measurement creates a CSV row with timestamp, ARFCN, RX level, raw SNR, raw BER and CGI.
2. **GSM-MON-003A:** feed known `MON` lines and compare parsed field values with expected fixtures.
3. **GSM-MON-003B:** feed split or interleaved console lines and check correct buffering or documented rejection.
4. **GSM-MON-003C:** test VTY disconnection and restart without corrupting the CSV header.
5. **GSM-MON-003D:** validate explicit handling of malformed records and unexpected ranges; preserve the original log for review.

## Outstanding evidence

- Exact Pi Zero model and power-supply measurements.
- CP2102 wiring and measured logic levels.
- Full successful `osmocon` invocation with complete boot log.
- Python monitor execution logs, CSV sample, and automated test results.
- SIM and incoming-call handling results (not attempted yet).
- Relay/dummy-load integration test results (not attempted yet).

**Privacy note:** Exact Cell Global Identities (CGI), Location Area Codes (LAC), individual ARFCNs combined with reception strengths, and raw radio logs are excluded to avoid disclosing a potentially identifying radio-location fingerprint. Keep raw evidence private and do not commit it to a public repository.


## November 2026 – SIM and GSM Integration

### Overview

Extended the OsmocomBB setup on a Motorola C121 and Raspberry Pi to support SIM initialization, GSM network registration, and incoming-call detection.

### Debugging and verification

- Compared RX-only and TX-enabled firmware builds after observing different SIM initialization behavior.
- Traced SIM APDU errors through the firmware and host-side L1CTL interfaces.
- Investigated ACK handling and tested a change to SIM procedure-byte handling.
- Successfully observed GSM network registration and incoming-call notifications with the modified TX-enabled firmware.
- Verified two GPIO-controlled relays using 200 ms test pulses.
- Implemented a prototype Python caller whitelist and relay-control state machine.

### Current limitations

- The exact contribution of the SIM driver change has not been isolated through a controlled regression test.
- Automatic startup using systemd is not yet reliable.
- End-to-end call-to-relay activation has not been consistently verified.
- Physical gate operation and unattended automatic closing are outside the verified scope.

### SIM ACK Handling Investigation

During integration testing, the TX-enabled firmware initially failed to read the SIM. Debug logs indicated a failure in the SIM ACK reception state.

I investigated the issue using an existing patch from the OsmocomBB `baseband-devel` mailing list, which addresses SIM cards sending the `0x60` NULL procedure byte before an ACK.

The following changes were tested in `calypso/sim.c`:

```c
sim_ignore_waiting_char = 1;  // Before receiving ACK
sim_ignore_waiting_char = 0;  // After ACK reception
```

After rebuilding the firmware, GSM network registration and incoming-call detection were successfully observed. However, the patch's individual contribution has not yet been isolated through a controlled regression test.

**Reference:** Etienne, *[patch] sim.c waiting char issue*, OsmocomBB baseband-devel mailing list, July 28, 2015. https://lists.osmocom.org/hyperkitty/list/baseband-devel@lists.osmocom.org/latest?count=100&page=5

### Next steps

- Add automated Python tests for call-event parsing and timer behavior.
- Improve startup sequencing and fault recovery.
- Capture reproducible integration test results.