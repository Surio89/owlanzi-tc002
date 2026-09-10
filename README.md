# Owlanzi TC002 App

A standalone Owlanzi application for the **Ulanzi TC002**. It runs on the clock
and queries the Owlet cloud directly. Normal operation requires no computer,
Home Assistant, MQTT broker, or additional server.

**Development version: 0.2.2.** The native desktop and ARM/FlyThings applications
share the same C++17 logic. The application has run on a real TC002; the setup
display, Owlet sign-in, and cloud polling while charging have been checked.
Full hardware acceptance and permanent installation are still pending.

The clock complements the Owlet base station. It replaces neither the base
station nor its alarms and is not a certified medical device.

## Try it locally

Windows builds and tests require Visual Studio Build Tools with C++ and CMake,
Python 3, and Node.js. These tools are available on the existing development PC.

```powershell
./scripts/build-local.ps1 -Run
```

Open `http://127.0.0.1:8080` without a password or setup key. Press `Ctrl+C` to
stop. The simulator uses sample data only and never contacts the Owlet cloud,
even if credentials are entered. The interface can simulate readings, charging,
waiting, offline states, and alarms. **Simulate brightness** changes only the
browser preview; it does not change the clock's brightness.

After building, you can start it directly:

```powershell
./build/owlanzi-tc002.exe --demo
```

On Linux, for development or CI:

```sh
# C++17 compiler, CMake, Python 3, libcurl development headers, Node.js
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
node --test tests/web-tests.mjs
./build/owlanzi-tc002 --demo
```

Starting explicitly with `--live --data-dir .local/live` enables real cloud
requests once an account is saved in the local interface. This repository's
development checks do not use that mode.

## Features

An interactive preview of the planned browser installer is available at
[installer-demo/dist/index.html](installer-demo/dist/index.html). Open it directly
in a browser or run `node installer-demo/serve.mjs` to serve it locally on port
8091. All connections and installation steps are simulated. See
[installer-demo/README.md](installer-demo/README.md) for the planned local helper.

- Direct Owlet sign-in for Europe and the international region, token renewal,
  and explicit device selection when multiple devices are paired.
- Heart rate, oxygen, sleep state, and sock battery on a custom 52×16 display.
- Time beside the readings in the lower right, automatic internet time, time
  zones with daylight saving rules, and manual adjustment. The large charging
  battery display remains available. See [CLOCK.md](docs/CLOCK.md).
- Freshness based on cloud measurement timestamps, a new session after charging
  or removal, stale-reading suppression, and retention of the last critical alarm.
- Owlet alarm notices and optional user threshold rules, disabled by default.
  Acknowledge alarms through the web interface or a hardware button.
- Local English/German interface, live display mirror, colors, brightness,
  sound, polling interval, and account setup.
- TC002 adapter for MCU initialization, SPI matrix, buttons/rotary control, and PCM audio.
- Persistent settings with atomic configuration replacement; directory permissions
  `0700` and file permissions `0600` on Linux.

The application provides its own Wi-Fi setup through the open `owlanzi` hotspot
at `http://192.168.4.1`. Enter and save Wi-Fi and Owlet details together before
the clock switches to the home network. Existing Wi-Fi profiles are reused.
See [WIFI_ONBOARDING.md](docs/WIFI_ONBOARDING.md).

The temporary ADB installer still requires a reachable clock. Initial installation
without manufacturer onboarding and permanent installation remain unfinished.
**OTA app updates** with daily checks, preserved settings, and rollback have
been tested on the TC002 and are available from owlanzi.com. See
[OTA.md](docs/OTA.md) for setup and limitations. The additional TC002 channel uses
the existing aggregate statistics; it does not replace the TC001 installer or
OTA channel or require changes to other website content.

## Build and test on the TC002

The official Windows cross-compiler and required FlyThings packages are downloaded
locally and verified against pinned SHA-256 hashes. See
[TC002_PLATFORM.md](docs/TC002_PLATFORM.md) for details and exact commands.

The result is a native ARM library, `libzkgui.so`, with the required UI resources
and a package manifest. It is **not an ESP32 image** and must not be installed
with the TC001 web flasher or its OTA feature.

The first hardware test uses the temporary FlyThings debugging workflow only.
See [LOCAL_TESTING.md](docs/LOCAL_TESTING.md) for steps and acceptance criteria.
The application first ran temporarily on a TC002 on September 9, 2026; the web
API and persistence work, and the setup display was confirmed on the matrix.
Colors/geometry, buttons, audio, and measurements while wearing the sock still
need practical verification. Permanent installation needs separate power-loss,
update, and recovery tests.

## TC001-style interface

Status, Display, Alarms, and System use the TC001 layout and branding. The 52×16
live mirror and color previews use the same TC002 renderer. Previews on the clock
end after ten seconds, and critical alarms take priority. See
[UI_PARITY.md](docs/UI_PARITY.md) for details and intentional platform differences.

## Repository layout

| Area | Responsibility |
| --- | --- |
| `src/core.cpp`, `src/render.cpp` | Platform-independent state and display |
| `src/owlet.cpp`, `src/http.cpp` | Owlet protocol and HTTPS with certificate verification |
| `src/config.cpp`, `src/runtime.cpp` | Persistence, threads, local web API |
| `web/` | Embedded interface without a CDN or external assets |
| `platform/tc002/` | Native FlyThings integration for the TC002 |
| `tests/` | Offline regression and integration tests |
| `.cache/`, `build/`, `.local/` | Ignored dependencies, builds, and private runtime data |

The interface initially opens without a password. An optional web password can
be set under **System → Access**; the username is `owlanzi`. There is no setup key.
APIs never return stored passwords or cloud tokens, and browser write requests
are restricted to the same origin. The TC002 serves the local interface over
HTTP on ports `80` and `8080`. It is intended for the home network and needs no
port forwarding. The desktop application binds to `127.0.0.1` by default.
On Windows, the selected local data directory's permissions apply.

## License

**GPL-3.0-or-later**; see [LICENSE](LICENSE). This license applies to this standalone
repository. The existing TC001 repository retains its own license. The project
owner authorized this separate port of Owlanzi behavior and parts of its existing
implementation.

Dependencies retain their own licenses; see
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). The manufacturer SDK is acquired
separately and is not presented as source code belonging to this repository.
Public binary distribution requires checking the redistribution terms of the
actual linked SDK components and providing the required sources. See the
third-party notices and OTA documentation for the current package contents.
