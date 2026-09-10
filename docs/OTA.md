# TC002 app updates

By default, the TC002 checks owlanzi.com for a new app version once a day. Under
**System → App updates**, you can disable daily checks, check manually, and
install an available update. Installation starts only after confirmation. Wi-Fi,
the Owlet account, display colors, brightness, alarms, time zone, and web password
are preserved.

## Current local installation

Version 0.2.x is an app update for the tested TC002 running Z21/stock firmware
1.0.1. It is neither an `update.img` nor a TC001 ESP32 image. A complete power
loss still returns to the manufacturer app. A permanent boot entry is separate,
unfinished work. Both app versions and settings remain stored persistently in
`/data`. Restarting the app during OTA requires no computer.

Set up the OTA launcher once after a successful local test installation, using
Python and ADB on the computer:

```powershell
python scripts/build-tc002.py --offline --device-abi-dir .local/device-192.168.100.235/abi
python scripts/bootstrap-ota.py install --ip 192.168.100.235 --adb <path-to-adb.exe>
```

After a complete power loss, resume the last confirmed version:

```powershell
python scripts/bootstrap-ota.py resume --ip 192.168.100.235 --adb <path-to-adb.exe>
```

The launcher checks the board, peripherals, available library symbols, and space
for two versions. It installs only under `/data/owlanzi-app` and switches the
temporary `/tmp/EasyUI.cfg`. It does not flash partitions.

## Updates and rollback

- Metadata: `https://owlanzi.com/firmware/ota-tc002.json`.
- Images: `owlanzi-tc002-VERSION-ota.bin`, at most 4 MiB.
- Separate package type `tc002-app-bundle`, ABI `z21-stock-1`, loader protocol 1.
- Fixed file list: `lib/libzkgui.so`, `ui/main.ftu`, `ui/cacert.pem`.
- TLS verifies certificates and hostnames; no redirects or external URLs.
- Checks cover sizes, complete SHA-256, individual file hashes, ARM hard-float
  ELF format, and files read back after writing.
- Downloads are temporarily stored under `/tmp`. Only the inactive slot `a` or
  `b` is written. User data under `/data/owlanzi` and `/data/misc/wifi` is never
  included in the package.
- An independent launcher switches the app. After ten seconds of display
  operation, the new version must regularly confirm its slot and version.
  If confirmation is missing for 45 seconds, the launcher restores the previous app.
- **Restore previous version** also allows manual rollback. Installation is blocked
  during a critical alarm or without home Wi-Fi. A new alarm or Wi-Fi loss cancels
  an ongoing download.

Hashes ensure integrity within the HTTPS-authenticated update channel. There is
no independent offline signature or secure-boot chain yet. The rollback guarantee
covers failed app startup, not failure of the entire Linux system or power loss.

## Daily checks and counting

The first automatic check runs no earlier than 60 seconds after startup, with
valid internet/device time and a home Wi-Fi connection. The Europe/Berlin day
is saved in `update-check.json` **before** the request. A restart, clock rollback,
or failed request therefore does not repeat the daily marker. Manually configured
display time does not affect this check. Manual checks remain available.

Starting with app 0.2.2, automatic requests include `daily-update-check=1`,
`model=tc002`, and the installed app version as `version`. The website aggregates
daily checks, OTA checks, and downloads by filename, without device IDs, accounts,
cookies, or individual usage paths. Existing `DNT`, `Sec-GPC`, and `no_stats=1`
exclusions remain in place. Disabling daily checks stops automatic requests;
manual downloads are still counted.

## Packaging and publication

The following commands illustrate the original 0.2.1 publication. Published
versioned files are immutable; use a new version for a new release.

```powershell
python scripts/build-tc002.py --offline --app-version 0.2.1 --device-abi-dir .local/device-192.168.100.235/abi
python scripts/package-ota.py
# In the separate website repository:
python website-tools/deploy-tc002.py
python website-tools/deploy-tc002.py --publish
```

The packager writes the image, metadata, and GPL source archive under `dist/ota`.
Only explicit source directories are included. Device libraries, credentials,
SDK archives, build directories, and local working data are excluded. The linker
map and `/licenses.txt` document included components.

The publisher uses SSH host `allinkl`, creates a local plan first, rechecks all
live hashes, backs up existing files under the HTTP-protected `.release-history`,
and switches the TC002 manifest last. Published versioned files are immutable.
TC001/ESP32 manifests, website content, and operator configuration are checked
for preservation.
