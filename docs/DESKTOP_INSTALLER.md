# Cross-platform TC002 setup

## 1.0.0 — stable release

The owner confirmed that the installer now works correctly and requested the
final release on 2026-09-13. Version 1.0.0 promotes the 0.1.6 installation flow:
device installation, discovery, account handling and update logic are unchanged.
The bundled TC002 app remains 0.3.2. Preview labels and the old browser-helper
alternative have been removed from the app and installation guide.

Builds use the checked-in `desktop/boot-payload.zip`, pinned by SHA-256. Its nine
app/boot files are byte-identical to the accepted 0.3.0 payload. It includes no
browser helper or host runtime; rebuilding no longer downloads a retired helper.
The app overlay still validates the 0.3.2 OTA and preserves the accepted boot.
Corresponding boot source remains linked in the bundled license notices.

Release acceptance is the owner's report, not a new hardware test performed by
the build jobs. Per-package `first_install_hardware_verified` remains false:
automated builds prove native startup and image construction, not installation
on physical hardware for every OS. Windows signing and Apple notarization remain
absent; the documented OS launch steps still apply.

## 0.1.1 — stock discovery fix and existing-clock updates

The original TC002 app 1.0.3 redirects `/uclockInfo.html` and other unknown
routes to `/settings/general`. The 0.1.0 search reached `/getBase` but rejected
that redirect, so both automatic discovery and manual IP identification failed.
0.1.1 probes the real `/settings/general` route and requires the combined stock
JSON schema and Ulanzi Clock HTML title. Static-page/root fallback remains for
other stock versions; redirects stay disabled for all account/update requests.
The owner confirmed this failure on Windows and Mac. The canonical page and
identification fix were verified read-only against the actual stock 1.0.3 clock.

Every address on the selected nearby LAN ranges is checked concurrently, with
progress shown in the window. Local Area Connection interfaces are retained;
virtual/VPN interfaces remain excluded. Manual entry also accepts a pasted
HTTP WebUI URL. No clock IP is embedded in production code.

Already installed Owlanzi clocks are recognized on ports 80/8080, including
password-protected WebUIs. Selecting one shows “App updates for this clock”.
The native update dialog uses `/api/update/check` and `/api/update/install` on
that clock. It shows installed/latest versions, checks a newer release and
requires explicit confirmation. The clock's existing OTA service handles its
download, hash validation and application activation. Network interruption
after an install request triggers status polling, never a second install POST.
Success requires the expected version to report itself after restart. Config,
Wi-Fi and resource partitions are not rewritten by this dialog.

Regression coverage: stock 1.0.3 redirects, older static UIs, unrelated HTTP
servers, both Owlanzi ports/auth, local interface naming and pasted URLs; update
version/busy/alarm/network gates, uncertain replies and real-version confirmation.
`--discover-to PATH [--address IP_OR_URL]` tests the actual frozen application's
read-only discovery without starting the wizard or installing anything.

Requested flow: open the desktop app, find the clock on the home network, select
it, install with a verified local backup, enter Owlet details, verify connection.
The clock continues independently when the app closes. Existing settings survive.
An IP address is a fallback, not the normal first step. Multiple clocks require
selection; discovery never installs or changes configuration.

Implementation uses a native Qt/PySide6 wizard and the existing tested Python
installation controller. A portable TCP ADB transport replaces the Windows ADB
executable. A local image builder must preserve every original resource and mode
and retain the existing RES-only write guard and readback checks. Credentials are
sent only to the selected, reidentified TC002, never to an Owlanzi service or log.
The helper does not implement a separate Owlet login: the clock checks the account
and supplies the paired-device choices. Password fields are cleared after sending.

Windows, Linux and macOS builds include their Python/Qt runtime. Each platform
must be built and smoke-tested on its native runner. The owner has a Mac for
acceptance; Linux is available in WSL for backend checks. Neither a native build
nor offline tests alone establish a successful first-time hardware installation.
The accepted 0.3.0 app/boot/guard payloads remain immutable. A desktop installer
release has a separate version and acceptance receipt.

Desktop preview 0.1.0 is built from commit `3c3251d` on the separate
`desktop-installer` branch. All four native jobs passed:
https://github.com/Surio89/owlanzi-tc002/actions/runs/34682239114
The previous Windows helper remains the hardware-accepted first-install route.

Delivered: Windows x64 ZIP, macOS Apple silicon ZIP, macOS Intel ZIP and Linux
x64 tar.gz, each with runtime, original license notices and exact installer
source. Linux/macOS also include native mksquashfs and its corresponding source.
Windows fetches the previously pinned manufacturer tools only when preparing a
new installation. No ADB executable or system Python is needed by an end user.

Validation on 2026-09-12:
- 12 discovery/account/container tests and 3 native GUI tests, on each OS.
- Frozen executable creates its GUI and verifies the immutable app payload on
  each native runner. Unix packages additionally verify the bundled packer.
- 20 existing installer/controller/image regression tests remain green.
- Automatic discovery found the real TC002 in 16 seconds without specifying
  its address. Read-only status confirmed the existing Owlet connection.
- The native Python ADB transport passed the real board/resource signature
  check and pulled a small system file. No device writes were performed.
- Native Linux image preparation retained all 233 original resource entries and
  their modes; the full 8 MiB original partition survived container round-trip.
- A synthetic account request was accepted by the real native application HTTP
  guard in an isolated loopback instance, including its required request header.
- GUI screenshots at 800 × 730 verified the main action remains visible while
  secondary controls can scroll.

Remaining acceptance: real first installation with the new transport/packer,
real Mac launch after download/Gatekeeper and local-network permission, and
fresh account setup against Owlet on hardware. The owner has a Mac. Preview
status is visible in the app, README, website and download metadata. macOS is
ad-hoc signed only; Apple notarization and Windows developer signing are absent.
Do not turn `first_install_hardware_verified` on based only on these build tests.

Build on the target OS (Python 3.11):

```
python -m pip install -r desktop/requirements-build.txt
python -m unittest discover -s tests -p "desktop*tests.py" -v
python scripts/package-desktop.py --output .desktop-build
```

For macOS/Linux add `--mksquashfs /absolute/path/to/mksquashfs`. The workflow
records native dependencies and the build baseline (Windows 2022, Ubuntu 22.04,
macOS 15 and macOS 15 Intel). Each output directory must be new. The build pins
the accepted app ZIP SHA-256 and refuses changed payloads. The archive contains
the corresponding Git HEAD source, so build from a clean committed checkout.

Developer GUI preview: `python desktop/main.py --demo`.
Packaged startup check: `"Owlanzi Installer" --self-test /absolute/proof.json`.
That check uses an offscreen synthetic GUI and performs no network requests.

Public download metadata: `https://owlanzi.com/downloads/tc002-desktop.json`.
Scoped publisher: website project's `website-tools/publish-tc002-desktop.py`;
run its read-only plan, review the prepared files, then use `--publish`.

Primary references:
- https://www.pyinstaller.org/en/stable/usage.html#supporting-multiple-operating-systems
- https://doc.qt.io/qtforpython-6/deployment/deployment-pyinstaller.html
- https://adb-shell.readthedocs.io/en/stable/adb_shell.adb_device.html
- https://developer.flythings.cn/zh-hans/download.html
