# Building OsmocomBB on Raspberry Pi (Debian 12 ARM64)

This guide records the **source revisions and tools used in a successful host/firmware integration** with a Motorola C121. Commands below are based on the observed setup; steps whose exact successful invocation was not captured are explicitly identified.

## Tested environment

| Component | Value |
| --- | --- |
| Phone | **Motorola C121** (Compal E88) |
| Host OS | Debian GNU/Linux 12 (Bookworm), ARM64 (`aarch64`) |
| Host board | Raspberry Pi Zero W / Zero 2 W family; exact variant not yet confirmed |
| USB-UART | Silicon Labs **CP2102**, USB VID:PID `10c4:ea60` |
| Working serial interface | `/dev/ttyUSB0` |
| Pi onboard UART | `/dev/serial0` -> `/dev/ttyAMA0` (not used for the successful download) |
| Native GCC | `gcc (Debian 12.2.0-14+deb12u1) 12.2.0` |
| Firmware GCC | `arm-none-eabi-gcc (15:12.2.rel1-1) 12.2.1 20221205` |

### Recorded Git revisions

| Repository | Commit |
| --- | --- |
| [osmocom/osmocom-bb](https://github.com/osmocom/osmocom-bb) | `168b8c5fe49e84a8a8142e64519085b363b4982a` |
| [osmocom/libosmocore](https://github.com/osmocom/libosmocore) | `1b72c0288c3f6fcec916cc439623af1c1c4b5d4c` |
| [osmocom/libosmo-gprs](https://github.com/osmocom/libosmo-gprs) | `6763a0c6aeab0424e2e41a2d500df38f47846234` |

These hashes were read from the working Raspberry Pi checkouts on 2026-11-05. To record them again:

```bash
for repo in osmocom-bb libosmocore libosmo-gprs; do
    echo "=== $repo ==="
    git -C "$HOME/$repo" rev-parse HEAD
done
```

For a new clone, check out the recorded commit **inside the corresponding repository** (do not run the three hashes against a single repository):

```bash
cd ~/osmocom-bb
# git checkout 168b8c5fe49e84a8a8142e64519085b363b4982a
```

Repeat with the relevant hash for each dependency. The command is shown commented to avoid overwriting local work accidentally.

## Install baseline development tools

```bash
sudo apt update
sudo apt install -y \
  git build-essential autoconf automake libtool pkg-config \
  libtalloc-dev libpcsclite-dev libusb-1.0-0-dev \
  gcc-arm-none-eabi binutils-arm-none-eabi
```

Additional development libraries may be required by the chosen upstream commits. If `./configure` reports a missing dependency, resolve it before proceeding. Avoid treating warnings about obsolete Autoconf macros as fatal errors.

## Dependencies built from source

Debian Bookworm provided `libosmocore-dev` **1.7.0-3**. For the checked-out OsmocomBB version, `pkg-config` initially could not find `libosmoisdn`. After installing a newer `libosmocore` from source, the next blocking dependency was `libosmo-gprs-rlcmac`, supplied by the separately built `libosmo-gprs` project.

Build/install **libosmocore first**, then **libosmo-gprs**. Do not install OsmocomBB's bundled `src/shared/libosmocore` as the system library; it is not the external host dependency.

### 1. `libosmocore`

```bash
git clone https://github.com/osmocom/libosmocore.git ~/libosmocore
cd ~/libosmocore
git checkout 1b72c0288c3f6fcec916cc439623af1c1c4b5d4c
autoreconf -fi
./configure --prefix=/usr/local
make -j2
sudo make install
sudo ldconfig
```

### 2. `libosmo-gprs`

```bash
git clone https://github.com/osmocom/libosmo-gprs.git ~/libosmo-gprs
cd ~/libosmo-gprs
git checkout 6763a0c6aeab0424e2e41a2d500df38f47846234
autoreconf -fi
./configure --prefix=/usr/local
make -j2
sudo make install
sudo ldconfig
```

**Check the package configuration:**

```bash
pkg-config --modversion libosmocore
pkg-config --modversion libosmoisdn
pkg-config --modversion libosmo-gprs-rlcmac
```

Each should print a version. If a `.pc` file is present but not discovered, check `pkg-config --variable pc_path pkg-config` and the actual install prefix before changing `PKG_CONFIG_PATH`. System-packaged and `/usr/local` libraries can coexist; confirm the resolver is selecting the intended version.

## Build OsmocomBB host programs and firmware

```bash
git clone https://github.com/osmocom/osmocom-bb.git ~/osmocom-bb
cd ~/osmocom-bb
git checkout 168b8c5fe49e84a8a8142e64519085b363b4982a
cd src
make -j2
```

In the recorded integration, `make` initially generated Autotools support files before the relevant binaries were produced. The following explicit targets were used or recommended during troubleshooting:

```bash
cd ~/osmocom-bb/src
make osmocon -j2
make layer23 -j2
make firmware -j2 CROSS_HOST=arm-none-eabi
```

Verify the tools and firmware rather than assuming a successful `make` exit code means everything was built:

```bash
./host/osmocon/osmocon -h
ls -lh host/layer23/src/mobile/mobile
ls -lh target/firmware/board/compal_e88/layer1.compalram.bin
arm-none-eabi-gcc --version | head -n 1
```

The locally compiled `layer1.compalram.bin` was observed at approximately **62 KiB**; its size is not a substitute for verifying it is usable.

## UART identification and firmware loading

The Raspberry Pi onboard UART was configured and appeared as `/dev/serial0 -> ttyAMA0`, but that setup did **not** successfully complete the observed firmware download. Using a Silicon Labs CP2102 USB-to-UART adapter provided the working host interface:

```bash
ls -l /dev/serial0 /dev/ttyUSB* 2>/dev/null
lsusb
# Adapter was detected as 10c4:ea60 Silicon Labs CP210x
```

The OsmocomBB loader invocation under investigation was:

```bash
cd ~/osmocom-bb/src
./host/osmocon/osmocon \
  -p /dev/ttyUSB0 \
  -m c123xor \
  target/firmware/board/compal_e88/layer1.compalram.bin
```

An initial attempt produced `Received DOWNLOAD NACK` followed by `FTMTOOL`. A later attempt ran firmware that repeatedly logged `BAT-ADC` and charger measurements. The **exact final successful loader command and retry sequence were not captured**, so this command is a documented candidate, not a fully proven one-line recipe. Firmware is loaded into RAM; there is no need to overwrite the phone flash for this workflow.

Record connector pinout, actual serial logic voltages, and the adapter wiring before publishing a physical wiring diagram. Do not treat a CP2102 device's USB identification as proof of the voltage levels at a particular breakout's pins.

## Configure and start the `mobile` host application

The sample config in the recorded checkout is `~/osmocom-bb/doc/examples/mobile/mobile.cfg` (**not** `default.cfg`, and **not** `~/osmocom-bb/src/doc/...`).

```bash
mkdir -p ~/.osmocom/bb
cp ~/osmocom-bb/doc/examples/mobile/mobile.cfg \
   ~/.osmocom/bb/mobile.cfg
```

With `osmocon` still running in the first terminal:

```bash
cd ~/osmocom-bb/src
./host/layer23/src/mobile/mobile -c ~/.osmocom/bb/mobile.cfg
```

Connect from another local terminal:

```bash
sudo apt install -y telnet  # if missing
telnet 127.0.0.1 4247
```

Examples of observed VTY commands:

```text
show ms
show subscriber 1
show cell 1
enable
monitor network 1
```

The recorded experiment used **no SIM card**, so `show ms` reported `IMSI detached`, `A6 no SIM inserted`, and `C7 camped on any cell`; it did **not** demonstrate network registration or incoming calls.

## Known integration issues

| Observation / error | Diagnosis or outcome |
| --- | --- |
| `Package 'libosmoisdn' ... not found` | Debian 12 `libosmocore-dev` 1.7.0-3 did not satisfy the checked-out host build; built newer `libosmocore` from source. |
| `Package 'libosmo-gprs-rlcmac' ... not found` | Built `libosmo-gprs` from source. |
| Obsolete Autoconf macros | Warnings; these were not the blocking build errors. Running `autoupdate` may modify tracked files and was not required to solve dependencies. |
| `osmocon` source exists but executable absent | Host program had not yet been compiled; use the explicit `make osmocon` target and check the output file. |
| `/dev/serial0` produced no usable firmware handshake | Successful integration used the CP2102 on `/dev/ttyUSB0`; root cause of the onboard-UART behavior remains unconfirmed. |
| `Received DOWNLOAD NACK` / `FTMTOOL` | Observed in an early loader attempt; later execution worked, but exact corrective action not confirmed. |
| `Failed to parse ... mobile.cfg` | Install the repository's actual `doc/examples/mobile/mobile.cfg` into `~/.osmocom/bb/mobile.cfg`. |
| `telnet: command not found` | Install Debian's `telnet` package. |

## Related evidence

For the actual GSM cell-monitor output, status checks, and PASS/NOT RUN matrix, see [Verification and Integration](verification.md). This build guide intentionally avoids duplicating measurement evidence.
