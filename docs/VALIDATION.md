# Development verification on September 9, 2026

## Checks actually performed

- Ported the TC001 interface: four tabs, shared branding/CSS, 52×16 live mirror,
  18 colors, device/browser previews, panel tests, and optional web sign-in.
  Five CTest groups and eleven Node checks passed. HTTP integration covers open
  initial access, optional password protection, foreign-origin rejection,
  preserved stored configuration during previews, automatic preview expiry after
  ten seconds, alarm priority, and explicit reset.
- Tested the local simulator in a browser on desktop and at 390 pixels wide:
  English/German, tabs, saving brightness, panel preview, return to live display,
  and saving language. No captured JavaScript/CSP errors or horizontal overflow.
  Previews use the same C++ renderer as the clock.
- Installed the update on a real TC002 after ABI and SHA-256 verification.
  Unauthenticated status/configuration requests returned 200, no web password
  was set, and the existing Owlet password remained stored. After six requests:
  `connected=true`, no error, charging state, and 832 pixels. The previous app
  library remained only as a local rollback copy; no credentials were exported.
- Jannik performed real Owlet sign-in and device selection; subsequent cloud
  polling on the TC002 succeeded. While charging, optional average field `oxta`
  returned placeholder 255. The original range check therefore rejected the
  entire record. The parser now treats exactly that placeholder as an absent
  optional field. Primary measurement limits and alarm flags remain unchanged.
  Regressions cover charging, active state, preserved flags, and continued rejection
  of invalid values.
- Installed the fix on the clock and verified it by SHA-256 readback. Four subsequent
  requests showed `connected=true`, `cloud_fresh=true`, no error, and the `battery`
  screen in charging state. `vitals_fresh=false` is intentional while charging.
- First real TC002 startup, manufacturer app 1.0.1 and MCU V1.0.16: the temporary
  FlyThings application ran; the HTTP interface and authenticated status returned
  200, with `target=tc002`, `mode=live`, and 832 matrix pixels. It ran continuously
  for several minutes. Jannik confirmed the physical “Owlanzi Local Setup” display;
  pixel geometry/colors, input, and audio remained pending.
- Read back all four installed files and checked their manifest SHA-256 hashes.
  Saved unchanged configuration through the protected API, returning 200.
- Checked actual system libraries before startup. Resolved C++ version differences
  by linking against the device library read from the clock. Fixed a filesystem
  startup crash by replacing mixed GCC 8/9 filesystem calls with POSIX calls.
- Built the complete application with Windows/MSVC C++17, including the embedded
  interface and native HTTPS implementation.
- Five CTest groups passed: state core, renderer, configuration/Owlet client,
  real local HTTP API, and local device helper. Core and renderer contained 132
  and 12 explicit checks respectively. Device-helper tests use fixtures only.
- Ten Node tests passed for configuration, translations, display, preview brightness,
  password handling, and authenticated API requests.
- Browser checks against the actual desktop application at desktop and 390-pixel
  phone widths covered pairing, language switching, thresholds, saving, keeping
  an empty password unchanged, explicit clearing, alarm acknowledgement, and offline
  display. No browser errors, CSP violations, or horizontal overflow.
- Completed an ARMv7/EABI5 hard-float build with the official Z21 GCC 8.3 toolchain.
  All required application symbols were checked at link time (`-z,defs`).
  FlyThings startup functions are exported from `libzkgui.so`.
- Verified manifest sizes and SHA-256 hashes of all four device payload files.
  The ZIP contains exactly those files plus the manifest, with no runtime configuration.
- A separate local package verifier accepted the generated ARM package. Tests
  rejected modified files, other targets, and unexpected payload paths.
- The existing `owlanzi-firmware` and `owlanzi-website` repositories remained
  unchanged during those initial application checks.

## Significant failure cases covered

Cloud measurement time rather than numeric change; identical new readings; stale
or future timestamps; session transitions after charging/removal; 20-second data
expiry and 30-second offline display; last known critical alarms; acknowledgement
and new causes; user alarm durations across different measurement timestamps;
unknown sleep values; rejected numeric overflow and wrong JSON types; multiple-device
selection; token renewal; account changes; partial APP_ACTIVE failure;
cancellation between sign-in steps; protected API writes; atomic configuration;
and passwords omitted from APIs/logs.

## Web interface loading failure fixed on September 9, 2026

A failed initial request previously left all settings disabled indefinitely,
while successful status requests hid the error. This was reproduced in a browser
by intercepting `/api/config`. Initialization now loads configuration and defaults
sequentially, retries failures, and shows initialization errors independently of
live status. Previews are rendered sequentially. HTTP connections close after
one response so idle browsers cannot occupy all three server workers between
requests. Rejected write requests are answered after reading their content,
bounded to 16 KiB, so separately sent headers and bodies cannot close the connection
before the JSON response. API tests cover this and optional access protection for
the interface, assets, and write requests.

- All five CTest groups and eleven Node tests passed. The API test also covers
  three open browser connections plus another client.
- Load `tests/browser-startup-fault.js` as a navigation `initScript`, then run
  `window.checkSettingsRecovery()`: repeated 503 responses, visible error despite
  successful live status, and automatic recovery. Passed on the real TC002 at
  390 pixels wide.
- Run `tests/browser-ui-check.js` as a browser function on a separate `--demo`
  instance. Color selection, canvas pixels, temporary device display, saving,
  alarm switches and threshold, password visibility, and saving fictional Owlet
  credentials passed. The test refuses real devices.
- Installed the updated ARM package on the local TC002 and verified its transfer
  with SHA-256. All 18 color inputs loaded; a color change appeared in both the
  canvas and device framebuffer. Saved Display, toggled the alarm switch, and
  saved the original alarm configuration. Checked Owlet fields in the browser
  without overwriting credentials. Public configuration was identical before
  and after testing, and no test preview was left active. No horizontal overflow
  at 390 pixels.

Subsequent TC002 color mapping was checked against the real renderer for all
25 palette entries. Every color must be visible in its corresponding state;
newly separated text and values must not color unrelated areas. API tests also
cover migration of old saved colors and preservation of already separated colors.
`tests/browser-palette-check.js` checks every color input through to its canvas
pixel on a demo instance, retention of selected preview states, and unchanged
saved settings. The mobile layout was visually checked at 390 pixels wide.

## Wi-Fi onboarding

Added a dedicated Wi-Fi wizard; see [WIFI_ONBOARDING.md](WIFI_ONBOARDING.md) for
behavior and hardware limits. Six CTest groups and eleven Node tests passed.
HTTP tests also cover Wi-Fi access protection, foreign origins, invalid data,
alarm priority, captive-portal redirects, failed attempts, and unchanged Owlet
configuration. `tests/browser-wifi-check.js` passed on the local demo instance:
scan feedback, SSID selection, password visibility/clearing, connection,
wrong-password rollback, configuration isolation, and layout.

On the real TC002, checked ARM ABI and transfer hash, started the updated app,
and preserved existing Wi-Fi. HTTP worked on ports 80 and 8080; a scan found nine
networks. A hotspot opened for ten seconds returned to the previous Wi-Fi after
about 21 seconds total without an error. The final build ran in live mode with
Wi-Fi connected. Phone DHCP/DNS acceptance and entry of an actual new password
were not performed.

## Combined setup and time

Wi-Fi and Owlet are saved together in the hotspot before switching connections.
Time replaces the small battery in the readings view. Seven CTest groups,
eleven Node tests, and the mobile `tests/browser-clock-setup-check.js` check passed.
NTP synchronization, Europe/Berlin, and preservation of existing settings were
confirmed on the real TC002; Wi-Fi and Owlet were connected. See [CLOCK.md](CLOCK.md).

## OTA app updates on September 9, 2026

- Eight CTest groups and twelve Node tests passed. New checks cover strict
  version/manifest validation, reserving the daily marker before requesting,
  retention across restarts, clock rollback, disabled daily checks, manual checks,
  alarm/Wi-Fi blocking, cancellation during download, and manual rollback.
- Website: sixteen HTTP/statistics tests and three targeted publisher tests passed.
  Coverage includes TC002 downloads, daily counters, DNT/GPC/opt-out, HEAD, Range,
  file boundaries, and unchanged payload bytes.
- Real device workflow: OTA launcher 0.2.0 in slot `a`, device fetch of the HTTPS
  manifest from owlanzi.com, installation of 0.2.1 in slot `b`, and startup
  confirmation. The web interface reloads after the app switch. Mobile layout
  was checked at 390 pixels wide without horizontal overflow.
- Manual rollback to 0.2.0 succeeded. Automatic rollback was then tested with two
  intact app versions and an intentionally mismatched expected version. Missing
  confirmation returned to the previous app. No binary or user setting was
  damaged for this test.
- App configuration, including credentials, colors, brightness, alarms, and time
  zone, and Wi-Fi configuration had identical SHA-256 hashes before/after OTA.
  Only hashes were recorded. Wi-Fi, Owlet cloud, and NTP were connected.
- Package 0.2.1: 2,572,354 bytes; SHA-256
  `63ab47d2d9582577a8f6df79c8a5835b3541e7733a1aa19c5e1a30db773c6f22`.
  Metadata, image, and GPL source archive were published over SSH and then verified
  byte-for-byte over public HTTPS. Thirteen existing website/TC001 files retained
  their hashes. The release backup remains protected from HTTP access.
- Removed static SDK audio/FFmpeg dependencies. Custom PCM output uses the existing
  system library. A quiet test tone was triggered and the app continued running.
  App process stderr is not available in logcat; subjective volume and complete
  acoustic acceptance remain pending.

## Not yet performed

No real measurement series while wearing the sock and no physical measurement of
hardware output. Memory usage, system time/Wi-Fi after device reboot, MCU responses,
complete pixel/color verification, and audio still need acceptance. The manifest
marks `hardware_verified: false`.

No permanent image installation or full power-loss test. The verified app rollback
is not Linux/boot recovery. Remaining acceptance criteria are in
[LOCAL_TESTING.md](LOCAL_TESTING.md).

Package boundaries, included open-source components, and excluded SDK/system
libraries are documented in [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)
and [OTA.md](OTA.md).
