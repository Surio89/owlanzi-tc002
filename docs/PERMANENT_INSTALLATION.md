# Permanent TC002 installation — acceptance and development history

## Accepted candidate, 11 September 2026

Candidate4 passed the complete Windows helper flow, including one RES update,
clock-side recovery, full 8 MiB readback and a user-operated power cycle with USB
power connected. The helper reached `complete`. A separate post-boot readback
verified the exact app, boot loader, controller and RES bytes. Owlet reconnected;
Wi-Fi and Owlanzi settings were preserved. The owner confirmed rotary brightness
in both directions on this final build.

The confirmed WebUI manufacturer-return endpoint selected the retained Ulanzi
application; its real HTTP page and persistent selection marker were verified.
This test then restored the same Owlanzi slot without flashing. Owlet reconnected
and both settings files were preserved. The return check took 8.9 seconds.
Evidence: `post-boot-verification.json`, `rotary-verification.json` and
`final-manufacturer-return/verification.json` under the ignored candidate4 work
pointer. The clean candidate ZIP is promoted without changing any executable,
script or UI payload. Its acceptance file binds every packaged payload hash.

Use the guided Windows installer; raw RES images are device-specific backup/
development artifacts. Public instructions: https://owlanzi.com/setup-tc002.html.
The historical notes below describe earlier failed candidates and are superseded
by this acceptance. Long-term use, complete audio/display acceptance and a
battery-only startup on these final bytes have not been established.

The user requires Owlanzi to remain selected across restarts. The retained
manufacturer app may be selected only by an explicitly confirmed WebUI action.
Resetting Owlanzi settings must not change that selection.

## Latest recovery state, 11 September 2026

**Recovered device, latest checks:** the owner completed another factory reset
and joined the home WLAN. The actual Ulanzi interface and ADB were reachable at
192.168.100.235. RES was the original stock partition, SHA-256
`6fca162bc647cf4709b6c3273e9029e76f075ff1ec67982561a5ef8d73aef335`.
The permanent controller, Owlanzi settings and old installation receipt had
been erased. The private saved settings were restored to the same clock.

The corrected app `22c738d3ecc8d89a4fec5018f6938b23fb9a511dc1a951ce3f5c72e17f586c17`
passed a temporary WLAN trial: Owlet connected, brightness was 199 and the WLAN
configuration remained byte-identical. A separate local trial watchdog restored
the real manufacturer interface in 84.5 seconds, after which the same Owlanzi
app and Owlet connection were restored. This explicit development-test fallback
is not part of the permanent boot path.

A 15-second hotspot cycle passed on the clock. A local HTTP/interface recorder
observed the setup interface at 192.168.4.1, running hotspot services and automatic
return to home WLAN. Owlet reconnected and WLAN settings were unchanged. The
host's first ADB transfer after reconnect failed transiently; a subsequent
verified connection retrieved the local evidence and ended the trial watchdog.
No RES write occurred during any of these tests. Evidence directories under
the ignored `.local/latest-second-reset-work.txt` pointer: `temporary-network-trial`,
`trial-fallback-check`, and `hotspot-cycle`.

**Candidate3 failure history:** the normal Windows helper triggered
the authorized RES update once, then timed out waiting for reconnection after
five minutes. Ports 80/8080/5555 were unreachable, the local guard receipt and
host write-verification receipt are absent, and no replacement Owlanzi endpoint
was found on the home /24. ADB lists the network device offline and no USB device.
The actual flash outcome and the cause of failed reconnection are unknown.
Do not infer completion from the timeout or retry the write. Public installer
release remains blocked on hardware acceptance.

The owner reported “Offline / Verbinde” and confirmed that rotary brightness
still works, proving the app is responding. Holding/releasing the top middle
button did not open setup. A switch-off/on with USB still connected did not
recover WLAN; neither did a subsequent owner-confirmed switch-off with USB and
dock disconnected for ten seconds, followed by powered startup and a one-minute
wait. These observations do not supply device uptime or a flash readback. No
further repeated power cycles are planned. A fresh scan found no TC002 endpoint
on another home /24 address. The owner also checked the Android Wi-Fi list:
neither “owlanzi” nor “U-Clock” is visible. No suitable USB stick/adapter is
available for external-media diagnostics. `.local/check-candidate3-recovery.py` is prepared for a
restored connection: it reads the matching durable guard receipt first, compares
the actual candidate3 RES, and preserves settings privately. It also compares
block and character MTD readbacks if available, to investigate possible cached
block data without flushing caches or writing flash. That is a hypothesis, not
a confirmed cause; no device execution of this diagnostic has occurred yet.

An independent source defect has been reproduced and corrected: the optional
station scan previously prevented both automatic and explicitly requested setup
hotspots if it threw an exception. A hotspot is now attempted with an empty
network list, permitting manual SSID entry. A failed hotspot is still reported
as a failure. New regression cases fail on the old implementation and pass on
the correction; all 11 CTest and 14 WebUI tests pass. The ARM build and device ABI
checks pass. This is not yet proven to explain the hardware outage.
New app SHA-256: `22c738d3ecc8d89a4fec5018f6938b23fb9a511dc1a951ce3f5c72e17f586c17`.
The app is preserved separately and has passed the temporary trials above:
`.local/latest-network-recovery-trial.txt` points to the prepared bundle/receipt.
Candidate3's original ZIP and payload remain unchanged.

Private backups were rechecked against candidate3's preparation receipt,
including complete RES and restore-image hashes and nonempty Wi-Fi/Owlanzi
configuration files. Evidence: `recovery-backup-audit.json` in candidate3's work
directory. The owner's subsequent reset restored access but erased the old guard
receipt, so it cannot now establish candidate3's exact flash or remount outcome.

Inspection of the exact installed manufacturer updater found another concrete
defect in our guard: `zk_upgrade_ready` removes `startupLibPath` and `lowMemMode`
from the temporary configuration before restarting its own upgrade UI (ARM
instructions around 0x6a14 and 0x6a34). The old guard resumed that modified file.
It now retains the validated Owlanzi slot configuration before the trigger,
restores it atomically after verified readback/remount, and reports each restart
preparation result. The host rejects receipts missing those confirmations;
20 installer unit tests pass. The corrected guard, SHA-256
`b6a51d501c111d2b33a6a64873092f374ce166f9d0757c5c60d3d8126bfc2109`,
passed a hardware test with the rewritten configuration and a stopped
`wpa_supplicant` service. Owlanzi/Owlet returned, the exact start configuration
was restored, and full RES/WLAN readbacks were unchanged. The vendor's driver
unload and an actual flash were not reproduced by this test. The initial
standalone SDK test helper failed before execution because of loader/dependency
issues; those attempts are not successful radio tests. The final small helper
used only the service-stop and completion properties, never the upgrader.
Evidence: the `latest-updater-resume-trial.txt` pointer inside the second-reset
work directory leads to the matching `verification.json` and guard receipt.
A successful simulation alone does not validate a permanent installation.

Candidate4 uses the corrected app and guard. ZIP SHA-256:
`96c9a6c80ff5114f9e17e4a7190af7eacc9bbcdcddb04ae017856151e710408a`.
Its normal Windows helper verified the real device, made fresh private backups,
and prepared a RES-only image retaining 233 original entries and their modes.
Prepared image SHA-256:
`8137be84be188f2b232b77fa5492d3a88881b7225f3392f22aadf681d0ead1ca`.
The latest rotary setting, 194, matched the fresh backup; all temporary trial
watchdogs were confirmed stopped before installation. The usual UI confirmation
was accepted although browser automation timed out afterwards. Read-only helper
status proved the operation was already running; no second start was requested.
The upgrade trigger was issued once and the normal helper reached `power_cycle`.
The matching clock-side receipt confirms write verification, RES remount,
start-configuration restoration and app restart. The host read back all 8 MiB;
the prepared image prefix matches exactly. Installed boot components and app
bytes match candidate4, Owlet connected, and Wi-Fi/settings including brightness
194 are preserved. Full written RES SHA-256:
`3c4894278a8243a96a77da75f65c40e1931b7b6d299d731324805d95c0dd7615`.
The owner completed the post-write side-switch restart with USB power connected.
Startup and final permanent manufacturer-return checks passed as recorded above.
Work pointer:
`.local/latest-candidate4-work.txt`.

The following successful checks predate the candidate3 write:

The owner has recovered Wi-Fi and confirms physical rotary brightness in both
directions. The corrected app, SHA-256
`42ae8d4aeb556f6a6549bd03bc885dd83e5286d3fdf12f7365ec1b59b3196cea`,
boots persistently, synchronizes system UTC as well as displayed time, and
connects to Owlet. The latest successful cold boot followed a restart with USB
power connected. Settings and Wi-Fi were preserved, and brightness 199 survived.
An earlier battery-only restart remained at a Ulanzi logo without networking;
its cause is still unproven. The logo alone does not prove manufacturer selection.

The installed 8 MiB RES was read back exactly after that successful cold boot:
`b91525109d2ce84eab57334a6378ab42559bc6782917a0bd499c377e53bae00b`.
It contains the earlier permanent boot controller. The original stock RES and
private settings/Wi-Fi backups remain in the ignored local device directory.

The real WebUI manufacturer-selection action returned 202 and selected the
manufacturer library/marker, but its HTTP server failed to bind port 80.
A corrected **temporary** controller waits for that port to become bindable
without SO_REUSEADDR before starting the retained app. In the corrected test it
waited 62.2 seconds, selected `/res/lib/libzkgui.so`, and served the real
`Ulanzi Clock - Settings` page. Returning to the saved Owlanzi slot restored Owlet
and removed the manual manufacturer marker. Settings and Wi-Fi were byte-identical.
Controller SHA-256:
`2d33ee3c2e2d5d114353855852b69e48dcdf8bc21647f260cebfa5527d70cd40`.
An earlier test that read the old page before the switch completed was explicitly
marked invalid; do not use that receipt as acceptance.

The on-clock installer guard has now passed verification, a normal RES
unmount/remount, and automatic Owlanzi/Owlet recovery **without writing flash**.
Two faults were found and corrected in these tests: the legacy
`ANDROID_PROPERTY_WORKSPACE` descriptor must survive detaching, and the retained
`hciattach` service must stop because it executes from RES. The guard checks actual
property access in its detached child before declaring itself ready. It never
forces an unmount, writes a partition, software-reboots, or selects Ulanzi.
Guard SHA-256:
`06817398279390131bfa277c6f66e3136f66d4f69140da479d0751788a814f1e`.
The full manufacturer flash/WLAN-disconnect/reconnect path still needs acceptance.

The packer now supports updating existing Owlanzi boot components while retaining
manufacturer code, configuration, other resources and permissions. The Windows
helper compares installed boot bytes and prepares a RES update only when they
differ; otherwise it performs the existing app-only update. Nineteen installer
unit tests pass, including preservation boundaries and update routing. ARM build
and direct symbol checks pass. Earlier app validation remains 11 CTest / 14 WebUI.

Candidate3 is packaged and its actual Windows UI has completed tool preparation,
compatibility checks, private backups and image preparation. Its reviewed image
changes **only** `bin/owlanzi-boot-control` among 236 existing resource entries.
Image SHA-256:
`d176872a8d9a80abc4332b35d977d4a474daffefea86af1e4062cb0380fc0e12`.
Windows ZIP SHA-256:
`88b9fd2854d8dab78793ba5d8286d4c2243380dc10297617b158051e6c61d3dc`.
After the original automatic-review rejection, the owner explicitly renewed
approval with “OK fortsetzen” and “continue”. A direct API start was separately
rejected as bypassing the hanging browser confirmation and was not executed.
The normal confirmation worked in a fresh browser tab using its Return key.
The unmodified Windows helper issued the upgrade trigger once, but ended in its
`recovery` phase with “Upgrade completion not confirmed”. No guard receipt could
be retrieved and no host readback was possible. The private helper work directory
contains `installer-recovery-state.json` and `install/upgrade-triggered.json`.
Write verification and the subsequent cold boot are still pending. Do not promote
the download before acceptance.

Evidence pointers under ignored
`.local/device-192.168.100.235/post-factory-reset-1789137242/`:
`latest-usb-boot-work.txt`, `latest-manufacturer-port-check.txt`,
`latest-guard-check.txt`, and `candidate3-readiness.json`.
`.local/latest-candidate3-work.txt` points to the new helper backup/prepared image.

The public EN/DE setup pages include the confirmed Android forget/rejoin/
Incognito/HTTP workaround and explicitly require a USB power adapter while data
transfer uses Wi-Fi. USB power wording was published and HTTPS verified at
2026-09-11T16:18:48Z. No TC002 installer download has been released.
Earlier entries below are historical and do not validate subsequent builds.

## Implemented locally

- A RES boot entry and independent start controller select the app slot stored
  under `/data/owlanzi-app`. Original Ulanzi resources are retained.
- `/api/system` exposes capability information. Confirmed manufacturer selection
  requires the normal same-origin protections and optional web password.
  Critical alarms and active app updates block those actions.
- The image preparer uses the manufacturer's RES-only package format and creates
  a local restore image from the owner's original resource partition.
- The device helper verifies transfers, checks the app ABI and starts a temporary
  app trial before triggering the permanent upgrade.

## Hardware test result, 11 September 2026

The app trial of 0.3.0 and the authorized RES upgrade succeeded. After the owner
manually restarted the clock, ports 80, 8080 and 5555 were reachable. Owlanzi
0.3.0 was live with a fresh uptime; `/res/etc/EasyUI.cfg` selected the permanent
boot entry and `/tmp/EasyUI.cfg` selected the saved Owlanzi slot. This confirms
persistent startup after that manual restart.

The rotary-input correction was then installed in slot b and passed application
startup/readback. Wi-Fi was byte-identical before/after this app update; only
brightness changed in the Owlanzi settings. Physical knob acceptance still needs
the owner's confirmation. A subsequent software reboot again left the clock
unreachable. The software reboot button, API and command have been removed from
the current source. That cleaned build is awaiting installation on the clock.

The original 8 MiB RES partition, extracted files and restoration image remain
in the ignored local device directory. The initial RES backup is not a full
user-data backup. Separate private Wi-Fi and Owlanzi settings backups were made
before the rotary-input update; they must never enter release archives.

An independent SquashFS inspection of the tested image confirmed retention of
the original files and modes except for the intended startup configuration.
The boot components' direct strong ELF imports matched the previously pulled
device system libraries. New boot-controller refinements require another build
and hardware check; the previously successful boot does not validate new bytes.

The installer now supplies the officially documented `zkrebootdelay` value `-1`
to disable the manufacturer's automatic software reboot. The supported upgrader
library is pinned by SHA-256. The helper resets and observes `sys.zkupgrade.state`,
then reads the entire RES partition back and verifies the prepared SquashFS bytes.
Only after this verification does it ask the owner to use the side power switch
to turn the clock off for five seconds, back on and verify startup. The TC002
has a battery: unplugging USB alone does not turn it off. The reset pin must
not be used for this step. The revised path is **not yet hardware
verified**. Its timeout, error, tampering and readback-failure paths have host tests.

Current connectivity checks still time out at `192.168.100.235`. Wait for the
owner's display/IP information or manual power cycle. Do not retry flashing
blindly. Test the explicit manufacturer selection and return to Owlanzi without
another software reboot before the final controlled installation test.

Follow-up diagnosis: the owner's quoted “Owlet: not connected” / “clock is
unreachable” text is from the browser, not proof of the physical clock state or
an Owlet login failure. An Owlanzi interface search across the local /24 found
no replacement address. No further device writes were performed while offline.
Source review found that a failed initial WLAN enable was never retried; the
service now retries after the normal boot delay. A delayed-radio regression
test passes, but this remains an unconfirmed explanation for the real outage.
The WebUI now clears stale status/readings and marks Owlet unknown when the
clock cannot be reached. Both corrections await installation and hardware checks.

Further physical checks: the owner reports the LED panel showing “Offline /
Verbinde” and no brightness response from the knob. Disconnecting USB had not
restarted the battery-powered clock. The owner then confirmed switching it fully
off and on using the separate side power switch. Subsequent checks of ports
80/8080/5555 and an Owlanzi search on the home /24 still found no connection.
USB ADB also lists no device. The current PC has no Wi-Fi adapter.

The owner has been asked to hold and release the top middle button for at least
six seconds to try Owlanzi's temporary setup AP; this is not a factory reset.
The owner reports that nothing happens. The owner subsequently confirmed turning
the clock off, disconnecting USB and observing it restart, then display
“Offline / Verbinde” again. A fresh home-network Owlanzi search found no device;
ports 80 and 5555 at the previous address remained unreachable and USB ADB
listed no transport. A complete restart has therefore not resolved the fault.
USB-stick availability is still pending. The new input reader retries missing evdev nodes and
reopens them after driver errors; it builds and passes the device ABI check, but
has not yet been installed. Do not claim the real knob or WLAN fault is resolved.

The official TC002 manual, downloaded from Ulanzi's documentation page, labels
the side power switch separately from the reset hole (English diagram, PDF p.4).
The installer and EN/DE website guide now explain the switch and battery. Merely
unplugging USB is no longer presented as a restart.

A temporary USB-start candidate is prepared locally at
`dist/owlanzi-tc002-0.3.0-usb-recovery-local.zip`. It contains the current app,
UI, CA bundle and an `/mnt/usb1` EasyUI configuration, with no upgrade image or
automatic-upgrade marker. ZIP SHA-256:
`32e7b6096ae0b99fd2352254bb998b889caa02f67361009b50f71709756afcac`.
This external-media boot is described in the manufacturer's IDE guide, but
has not yet been demonstrated on this TC002. Do not present it as a verified
recovery or public installer. A separate bounded evdev diagnostic probe is
ready under `.local/input-probe` for use after network access returns.

The owner subsequently used the recessed reset button beside USB-C and now
reports the display showing `192.168.100.1`. This matches the manufacturer's
documented `U-Clock` setup hotspot address; the running app and partition state
have not yet been inspected. A fresh check still found no ADB USB transport or
present Windows USB device matching the TC002, and the previous home-network
address remained unreachable. Confirm the direct PC data-cable connection; if
USB remains unavailable, the manufacturer's setup page can rejoin the home WLAN
from a phone. Do not use `192.168.100.1` through the PC's Ethernet route as though
it were the clock: its home network uses the same subnet. Read back the device
identity, resources and remaining user data before choosing the repair path.

## Release procedure

Promote the accepted clean ZIP using `scripts/promote-installer.py`, retaining
tested payload bytes. Package the matching app-only OTA and corresponding source.
Publish the OTA/source channel first, then the Windows helper and EN/DE guides
using the website's scoped TC002 publishers. Both publishers create a read-only
plan, preserve existing TC001/operator/analytics content, back up replacements,
and verify public HTTPS responses. Never publish private device evidence.
