# Local testing with the first TC002

Initial status on September 9, 2026: temporary startup on a real TC002 succeeded.
The native web API responded in live mode and settings could be saved. Physical
output and Owlet sign-in had not yet been accepted at that stage; see
[VALIDATION.md](VALIDATION.md) for subsequent checks.

## Specifics of the first clock

Manufacturer app `1.0.1`, MCU `V1.0.16`, Z21/ARMv7, glibc 2.30. This firmware
returns no model field from `/getBase`. The helper therefore checks the
manufacturer response schema, board/hardware, static Ulanzi Clock interface,
and expected SPI, UART, and input devices together. Legacy ADB does not report
remote failures through its process exit status, so the helper reads an explicit
shell status. Files are read back and hashed locally because `sha256sum` is absent.

If ABI verification stops at `exception_ptr` symbols, the required device
libraries have already been copied to `.local/device-<IP>/abi`. Build with:

```powershell
python scripts/build-tc002.py --offline --device-abi-dir .local/device-<IP>/abi
python scripts/local-device.py run --ip <IP>
```

This links against the clock's actual `libstdc++.so.6` without distributing or
replacing it. Linux persistence uses POSIX calls to avoid incompatible GCC 8/9
filesystem types. If startup verification fails after activation, the helper
restores the manufacturer application automatically.

## On the computer

`scripts/build-local.ps1 -Run` builds, tests, and starts the isolated simulator.
Open `http://127.0.0.1:8080` directly, without a pairing step.

Check readings, charging, unknown sleep state, transition to offline, alarm
acknowledgement, account settings, keeping an empty password unchanged or clearing
it explicitly, English/German views, brightness zero, and narrow screens.
Simulation is clearly marked and never makes cloud requests.

## When the clock is available

1. Record the Ulanzi firmware version and device model. The current temporary
   ADB installer requires the clock and computer to be on the same local network.
   Once installed, Owlanzi can configure Wi-Fi itself. Initial installation on
   a device that has not been set up remains separate work.
2. Check original functionality and the documented recovery path before replacing
   the device application. A reset may erase saved settings, so check the actual
   device version and manufacturer instructions first.
3. Use `scripts/local-device.py inspect --ip <IP>` to read model information over
   HTTP and Wi-Fi ADB. The script does not scan for other network devices.
4. Run `scripts/build-tc002.py` and inspect the manifest and ARM library.
5. `scripts/local-device.py run --ip <IP> --bundle build/tc002/device` starts the
   verified package temporarily. An unknown model response stops the helper;
   inspect the actual response before extending model matching. Before startup,
   it reads the required system libraries from fixed device directories and checks
   their exported symbols against the ARM application. Missing libraries or symbol
   versions block startup. SDK link stubs must never replace actual device files.
6. Open the local interface at `http://<IP>:8080` without a setup key. Under System,
   configure Wi-Fi if needed, then the Owlet account. A web password is optional;
   old pairing files are no longer used.
7. Use `scripts/local-device.py restore --ip <IP>` to remove the temporary Owlanzi
   application and restart the manufacturer app. A complete reboot also discards
   temporary files. Account data under `/data/owlanzi` is separate and remains
   available for the next test.

The helper requires Android Platform Tools (`adb`) on PATH or `--adb <path>`.
It writes no flash images or upgrade properties and refuses to replace another
application's debug configuration. ADB is a powerful development interface;
a separate verified workflow is planned for public initial installation.

## Acceptance criteria on real hardware

- **Startup and matrix:** Successful MCU version query, correct pixel order,
  all four corners correct, no shifted colors, and no flicker.
- **Controls:** Rotary control changes brightness in both directions;
  acknowledgement affects the same alarm as the web interface. Check left/right.
- **Audio:** Volume zero stays silent; short test sounds work; local/web
  acknowledgement stops output. Start with synthetic alarm data.
- **Wi-Fi and time:** Connection after reboot, valid UTC before HTTPS, restored
  connection after Wi-Fi interruption, and verified hostname/certificate.
- **Owlet:** EU/international sign-in, token renewal, and multiple-device selection;
  identical new readings remain valid, while cached old readings are hidden.
- **States:** Charge, remove, and put on the sock; turn off the base station;
  interrupt cloud access. Last critical notices remain identifiable as such.
- **Persistence:** Settings survive restart; interrupted writes leave a complete
  old or new file. Passwords are absent from API responses and logs. No secret
  appears in the build, manifest, or error report.
- **Resources:** Observe CPU/RAM, file descriptors, temperature, audio/MCU behavior,
  and the interface over at least several hours of operation.
- **Recovery:** Actually test restoration of the manufacturer app and behavior
  after a complete power loss.

## Remaining release work

A permanent `update.img`, automatic startup after power loss, and a public
initial-installation wizard remain unfinished. The separate TC002 OTA app channel
is implemented; see [OTA.md](OTA.md). Installation, manual rollback, and missing
startup confirmation were tested on the clock. This does not replace a power-loss
test of the full Linux/boot system.
