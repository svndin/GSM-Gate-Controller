# INT-001: L1CTL Power Measurement Interface Verification

A standalone C++17 parser and regression test for the `L1CTL_PM_CONF` message
used by OsmocomBB. No live radio traffic or modifications to OsmocomBB are
necessary.

## Provenance

Derived from the structures and host receive logic shared for the following
OsmocomBB revision:

- `osmocom-bb`: `168b8c5fe49e84a8a8142e64519085b363b4982a`
- Definition: `src/host/layer23/include/l1ctl_proto.h`
- Host handler: `src/host/layer23/src/common/l1ctl.c`, `rx_l1_pm_conf()`
- Device: Motorola C121; host: Raspberry Pi (Debian 12 ARM64)

This is an **independent defensive decoder**, not a direct test of OsmocomBB's
existing function. Integration tests of the original C handler are separate work.

## Build and run

```bash
sudo apt install -y cmake g++
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

Optional Doxygen documentation:

```bash
sudo apt install -y doxygen
doxygen -g Doxyfile
# Set INPUT = include src, RECURSIVE = YES, GENERATE_HTML = YES,
# and optionally EXTRACT_ALL = YES in Doxyfile.
doxygen Doxyfile
```

## What the tests cover

- Valid PM_CONF message (`0x09`)
- Header fields and measurement-field preservation
- Big-endian 16-bit `band_arfcn`
- Multiple packed 4-byte measurement records
- Empty and truncated header
- Wrong message type
- Missing measurement record
- Truncated measurement record and extra trailing byte

## Wire format assumed by this test

```text
L1CTL header (4 bytes):
  msg_type | flags | padding[0] | padding[1]
Each PM_CONF entry (4 bytes):
  band_arfcn high | band_arfcn low | pm[0] | pm[1]
```

The `band_arfcn` field is decoded in network byte order as implied by the
source's `ntohs()` call. `pm[0]` and `pm[1]` remain raw values and are not
converted into dBm. No socket transport-length prefix is included in these
inputs.

## Verification note

The supplied OsmocomBB host function checks only that the payload is at least
one record long before iterating to the end of the buffer. The independent
decoder here also rejects payload lengths that are not a multiple of four.
This is a **test-design observation**, not evidence of a demonstrated crash
or exploited defect in the original code. Confirm how upstream framing and
buffer validation work before filing a bug.
