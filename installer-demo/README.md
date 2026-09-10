# Local HTML installation demo

`dist/index.html` is a complete, standalone HTML file. It also works when opened
directly in a browser, without installing packages or accessing the internet.

Alternatively, from the repository:

```powershell
node installer-demo/serve.mjs
```

Open `http://127.0.0.1:8091`. The server listens locally only and serves the HTML
file. It has no API, ADB connection, or installation endpoint.

The demo covers preparation, starting a future installation helper, entering an
IP address, checks, temporary startup, and subsequent setup. All check results,
progress, and device displays are simulated. **Example scenario** lets you try an
unreachable clock, an incompatible system, and an unresponsive application.
Do not enter real credentials. Sample data is neither stored nor transmitted.

## Turning this into a real installer

1. A local helper, downloaded once, runs on the computer and serves the wizard
   at `127.0.0.1`. owlanzi.com could offer the download later; this demo does not
   change the website.
2. The user enters the TC002's IP address. The helper integrates the checks from
   `scripts/local-device.py`: model, Wi-Fi ADB, ARM interfaces, file list, and
   SHA-256. The current demo does not use that helper.
3. Approval tied to the IP, package hash, and current checks is created only when
   the user clicks the test-start button. The helper accepts no arbitrary commands,
   URLs, or file paths from the page. API requests are authenticated locally and
   restricted to the helper's own origin.
4. The helper transfers the verified package temporarily, starts it, and reports
   success only after an authenticated device-interface response. Diagnostics and
   targeted restoration remain accessible after errors.
5. The planned local flow passes its pairing code to the device interface without
   putting tokens in public URLs. Owlet credentials are entered only in the
   device interface.

This browser adapter for the helper is not implemented yet. The existing CLI
helper and ARM package provide its technical foundation. A permanent firmware
image and recovery after power loss need separate tests on a real clock.

Ordinary web pages cannot open arbitrary TCP connections for Wi-Fi ADB; the local
helper bridges that browser limitation. See
[Chrome: Direct Sockets](https://developer.chrome.com/docs/iwa/direct-sockets).
The native TC002 development workflow is documented in the
[official Ulanzi project](https://github.com/UlanziTechnology/Ulanzi-U-Clock-TC002).

License: GPL-3.0-or-later, like the app.
