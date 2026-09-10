# TC002 platform and device build

The device build is a native C++17 application for the TC002's existing
Linux/FlyThings system. `libzkgui.so` contains Owlet polling, configuration, the
local web interface, alarm rules, and display rendering. Once installed, it needs
no computer or external MQTT service. A TC001 ESP32 binary cannot run on this platform.

The initial build compiled and fully linked an ARM ELF with the official Z21
toolchain before hardware was available. Initial installation, matrix output,
audio, Wi-Fi reconnection, and startup after power loss were pending at that
stage. See [VALIDATION.md](VALIDATION.md) for later hardware results.

## Sources and pinned versions

The hardware basis is the [official Ulanzi project](https://github.com/UlanziTechnology/Ulanzi-U-Clock-TC002/tree/fa9d85d8e639430332c117cac71ce08dd6beb3f7),
commit `fa9d85d8e639430332c117cac71ce08dd6beb3f7`. Its
[hardware description](https://github.com/UlanziTechnology/Ulanzi-U-Clock-TC002/blob/fa9d85d8e639430332c117cac71ce08dd6beb3f7/Z21_TC002_Demo/README.md)
and [IDE/installation documentation](https://github.com/UlanziTechnology/Ulanzi-U-Clock-TC002/blob/fa9d85d8e639430332c117cac71ce08dd6beb3f7/IDE%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E/%E8%AF%B4%E6%98%8E%E6%96%87%E6%A1%A3.md)
describe the interfaces used.

`cmake/tc002-dependencies.json` pins the toolchain, FlyThings packages, and Mozilla
CA bundle with HTTPS sources and SHA-256 hashes. Downloads stay in
`.cache/tooling/`, outside Git. The script verifies archives before extraction
and stops if files have changed.

| Component | Version used |
| --- | --- |
| Official Z21 toolchain for Windows | Linaro GCC 8.3.0, `arm-pc-linux-gnueabihf` |
| CPU/ABI | ARMv7-A, NEON, EABI5, hard-float, unsigned `char` |
| FlyThings EasyUI | 2.6.0 |
| base-utility / audio-utility in the SDK | 10.9.3 / 5.1.1 |
| HTTPS | libcurl 8.12.1-mbedtls, mbedTLS 3.6.5 |
| DNS resolution in SDK curl | c-ares 1.17.2 |
| CA bundle | Mozilla/curl, 2026-08-13 |

These versions come from the manufacturer SDK. In particular, c-ares is old;
network libraries should be rebuilt from current, reviewed sources before broader
release. The SDK's alternative OpenSSL curl build targets OpenSSL 1.1 and cannot
be substituted with its OpenSSL 3 archive. This build uses the mbedTLS variant.

## Building

Build computer requirements: Windows, Python 3.11 or later, CMake 3.20 or later,
and Ninja. CMake and Ninja can also be found in an existing Visual Studio Build
Tools installation. No FlyThings IDE or driver installation is required.

```powershell
python scripts/build-tc002.py
```

The first invocation downloads pinned manufacturer packages and the toolchain,
configures CMake, compiles `zkgui`, checks all application references at link time,
and creates a local test package. Other options:

```powershell
python scripts/build-tc002.py --prepare
python scripts/build-tc002.py --offline
python scripts/build-tc002.py --cmake C:/Tools/CMake/bin/cmake.exe --ninja C:/Tools/ninja.exe
```

`--offline` uses extracted files without rechecking their archives. Run `--prepare`
first for a complete hash check. Inputs and tool versions are pinned, but
bit-for-bit reproducibility on a second computer has not been verified.

The manufacturer also documents a Linux SSD toolchain. The automated build
verified here uses the Windows Z21 toolchain; a Linux device build has not been checked.

## Startup, display, and controls

FlyThings loads `libzkgui.so` and calls `onEasyUIInit`, `onStartupApp`, and
`onEasyUIDeinit`. The initialization hook immediately sets the required manufacturer
property `sys.zkapp.state=running`. Application work then starts in a separate
thread; long network requests do not block the EasyUI event loop. A minimal
registered `mainActivity` uses the unchanged empty `main.ftu` from Ulanzi.

The device integration uses:

- MCU `/dev/ttyS1`, 1,500,000 baud, 8N1, no hardware flow control. After each
  startup, the version query `ff 55 11 00 01 65` must receive a successful response
  before matrix access. Response reads are bounded and checksummed.
- Logical 52×16 RGB888 pixels mapped to a 64×16 RGB SPI packet with black extra
  columns. SPI0 uses 10 MHz, mode 0. GPIO_35 is set to 0 before writing and to 1
  afterward. Frames are at least 16 ms apart.
- Inputs `/dev/input/event67` and `/dev/input/event68`. Clockwise rotation or the
  right button increases brightness; counterclockwise rotation or the left button
  decreases it. Pressing the rotary control or middle button acknowledges alarms.
- The original adapter used the public SDK `AudioPlayer` interface for a custom
  short double tone, mono/16 kHz/S16 PCM, and volume 0–6 following manufacturer
  mapping. The 0.2.x app uses the installed MI_AO interface instead; see
  [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md). Acknowledgement and disabled
  sound stop playback.
- Wi-Fi uses `WifiManager` in station mode and the existing `hostapd`/`dnsmasq`
  system programs for the application's open hotspot. SDK `wifi_load_driver()`
  loads the existing driver after station mode is disabled. Application AP
  configuration stays under `/data/owlanzi`; system programs are not replaced.
  See [WIFI_ONBOARDING.md](WIFI_ONBOARDING.md) for behavior and limits.

An unavailable panel does not prevent the local web interface from starting.
This allows diagnosis and setup during early hardware testing even when hardware
details differ. Startup errors are written to the process log without credentials.

## Data and local test files

The application defaults to `/data/owlanzi`; `OWLANZI_DATA_DIR` can override it
for a debug start. The SDK uses `/data` for persistent storage, including
`/data/misc/wifi/wpa_supplicant.conf`. Available capacity and retention after
manufacturer updates must be checked on the actual TC002. Configuration and
legacy pairing files do not belong in the application package.

A build produces the following layout; the ZIP name reflects its app version:

```text
build/tc002/device/
  EasyUI.cfg
  lib/libzkgui.so
  ui/main.ftu
  ui/cacert.pem
  manifest.json
build/tc002/owlanzi-tc002-app-0.2.2-local.zip
```

`EasyUI.cfg` is prepared for a temporary test under `/tmp/owlanzi-tc002-app`.
Creating it does not change any clock settings. `manifest.json` lists the four
package files with sizes and hashes, target platform, pending hardware checks,
and all system libraries required by the ELF. The ZIP is assembled from a fixed
file list, never from a working or configuration directory.

The downloaded FlyThings SDK's `.so` files are stubs for linking against the
existing system. They are **not** copied into the device package. In particular,
`libeasyui.so`, `liblog.so`, `libzkhardware.so`, `libzknet.so`, `libmi_ao.so`,
`libmi_sys.so`, `libmi_common.so`, `libcam_os_wrapper.so`, and the Linux/GCC runtime
libraries listed in the manifest must exist on the TC002 with a compatible ABI.
The local testing helper must verify this before startup.

The generated ELF requires versions including `GLIBC_2.28` and `GLIBCXX_3.4.22`;
`required_symbol_versions` contains the complete list. Correct system time is
also required for CA verification. Whether stock firmware synchronizes time
independently of the Ulanzi interface must be checked on the device. Certificate
failures are never bypassed by disabling TLS verification.

## Future permanent installation

The local package is not an `update.img` or a flashable firmware image. Manufacturer
documentation distinguishes temporary Wi-Fi ADB debugging from permanently installed
images. The permanent path describes IDE image creation and an ADB upgrade using
`sys.zkupgrade.flag`, `sys.zkupgrade.dir`, and a `zkswe` restart. The build script
performs none of those actions.

Verify temporary startup and return to the original software first. Permanent
image creation must then be checked against the actual firmware version. The
manufacturer documents physical recovery using the reset button beside USB-C,
but this has not been tested on this device. The generic TF-card workflow in the
IDE documentation does not establish that the TC002 has an accessible TF interface.

## Licensing and distribution

Owlanzi TC002 and the reused Ulanzi portions are GPL-3.0-or-later.
`platform/tc002/resources/main.ftu` was copied unchanged from the pinned GPL
Ulanzi project. The hardware adapters are new implementations based on its
documented protocols and wiring. Repository license texts and notices also
identify separately included open-source components.

The [manufacturer notices](https://github.com/UlanziTechnology/Ulanzi-U-Clock-TC002/blob/fa9d85d8e639430332c117cac71ce08dd6beb3f7/THIRD_PARTY_NOTICES.md)
do not provide complete terms for FlyThings proprietary packages. The initial
build linked some of these statically, requiring redistribution rights and any
corresponding source obligations to be resolved before public binary distribution.
The repository does not distribute SDK binaries; local builds download them from
the manufacturer. The 0.2.x package has since removed proprietary static utility
and audio dependencies. See [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md) for
the current linked components and notices; the GPL license for this project's
sources does not itself license the manufacturer SDK.
