# TC001 interface on the TC002

The local device interface adopts the TC001 logo, colors, typography, sidebar,
mobile navigation, cards, inputs, and save bars. This port does not modify the
TC001 source files or owlanzi.com.

| Area | TC002 implementation |
| --- | --- |
| Access | Initially open, with no sign-in or setup key; optional web password under System, username `owlanzi` |
| Status | Heart rate, oxygen, sock battery, sleep/charging state, base station, hardware, cloud connection, latest errors, request count, and uptime |
| Live mirror | Exact 52×16 pixels from the native renderer, with optional browser brightness simulation |
| Display | Normal and alarm brightness, separate preview brightness, and independent colors for readings, sleep, battery outline/levels, waiting, offline, and alarms |
| Color preview | Same C++ renderer for browser and clock; unsaved changes appear on the clock for ten seconds; individual and full palette reset |
| Panel tests | Four colored corners, a moving pixel across all 832 pixels, and full-panel color cycling |
| Alarms | User thresholds and durations, sound switch, volume 0–6, repetition, manual test sound, and acknowledgement |
| System | Wi-Fi scan, network switching and setup hotspot; Owlet account, real device selection, region, polling interval, English/German, optional web password, and Owlanzi settings reset |
| Persistence | Display, Alarms, and System are saved separately; unsaved changes in other sections remain in the form |

## Platform differences

The TC002 palette follows the elements actually rendered on its display. Its
25 colors control:

- Top readings row: heart, pulse value, O2 label, and oxygen value including the
  percent sign, each independently.
- Lower-right clock: HH:MM with its own color; time zone and manual/automatic time
  under System. See [CLOCK.md](CLOCK.md).
- Lower-left sleep state: text and indicator bar together for each sleep state.
- Battery: outline, normal/charging/medium/low fill, percentage, and the CHARGING
  and SOCK OFF labels separately. The charging view remains; the readings view
  now shows time beside the readings.
- Waiting/offline: heart without readings, O2 and placeholders, WAITING, OFFLINE,
  and RETRYING separately.
- Alarms and notices: both text rows together for each category. Setup has its own
  OWLANZI title color and uses the notice color for LOCAL SETUP.

O2 starts at x=27 in the readings view. The lower right shows time instead of the
small battery. The separator dot and its color setting have been removed; old
saved separator colors are ignored on load.

State switches on color cards show the corresponding real 52×16 layout. Editing
a color selects the state in which that element is visible. Other cards retain
their chosen preview. State switches alone affect only the browser; color edits
briefly activate the clock preview as before. Existing saved colors migrate to
newly separated elements on load, preserving colors already set independently.
A battery above 0% has at least one filled pixel column so that the low-battery
color remains visible.

- The TC002 uses 52×16 instead of 32×8 pixels. Sleep state and time share the second
  row; all tests use the complete new matrix.
- Volume follows the TC002 scale of 0–6. Brightness is controlled manually or with
  the rotary control; ESP32 light-sensor readings and thresholds are not simulated.
- Wi-Fi setup follows the TC001 approach: open `owlanzi` hotspot, `192.168.4.1`,
  network selection, and password. The TC002 alternates its single radio between
  hotspot and home Wi-Fi; the network list is cached in hotspot mode. See
  [WIFI_ONBOARDING.md](WIFI_ONBOARDING.md).
- The TC002 has no TC001 OTA/ESP32 flasher. Permanent installation and its recovery
  checks are separate work; TC002 app updates use the channel described in [OTA.md](OTA.md).
- Reset removes only Owlanzi configuration, including the Owlet and web passwords.
  System Wi-Fi and the manufacturer app remain intact. The UI requires confirmation.

## Previews and access

Browser previews change neither cloud state nor device settings. A device preview
is explicitly marked and ends automatically, on save, or through **Back to normal**.
A real critical alarm cancels the preview and blocks new ones. Sample values are
never written to the state core as cloud readings or real alarms.

Without a web password, the interface is directly accessible on the local network.
Once set, the password protects the page and API through HTTP Basic Auth.
Configuration responses omit passwords. JSON write requests still require the
application's request header; foreign-origin and cross-site requests are rejected.
No external scripts are loaded.

## Display defaults

The palette selected on the TC002 on September 9, 2026 is the default for initial
setup and color reset. Normal brightness is 8/255, preview brightness 15/255, and
alarm brightness 255/255. The adopted colors and added white clock color are in
`Palette` in `include/owlanzi/core.hpp`; `/api/defaults` exposes the same values
to the web interface. Updates retain existing saved settings.
