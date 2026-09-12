# Cross-platform TC002 setup

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

Status: implementation in progress; current public Windows helper remains the
accepted installer until the desktop release passes its checks.

Primary references:
- https://www.pyinstaller.org/en/stable/usage.html#supporting-multiple-operating-systems
- https://doc.qt.io/qtforpython-6/deployment/deployment-pyinstaller.html
- https://adb-shell.readthedocs.io/en/stable/adb_shell.adb_device.html
- https://developer.flythings.cn/zh-hans/download.html
