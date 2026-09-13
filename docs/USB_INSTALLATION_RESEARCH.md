# TC002 USB installation investigation

Investigation on the existing Z21 TC002, 2026-09-10. Initial inspection was
read-only. After the user confirmed the USB cable connection, two temporary
USB-role tests succeeded and restored the original host role. No upgrade,
reboot, reset, or persistent configuration write was performed.

## Confirmed USB cable result

USB ADB works on this physical TC002 and Windows PC after a temporary role
switch. Windows enumerated `18d1:d002` as `Zkswe` using its existing `WINUSB`
driver. `adb -d shell` (explicitly USB, not the IP transport) returned:

- `ro.product.model`: `Zkswe_SSD21X_SPINOR`
- `ro.product.board`: `swaio`
- A harmless echo probe: `OWLANZI_USB_READ_OK`

The operation follows the locally inspected manufacturer's `UsbSwitchHelper`:
write `22` to `otg_role`, wait 1.5 seconds, read the action node `usb_null`, then
read `usb_device`. These sysfs action reads **change** the USB role; they are not
read-only diagnostics. Restoration uses the same sequence ending in `usb_host`.
Both tests restored `usb_host`; a subsequent WLAN echo probe also succeeded.
The first test discovered that the device's BusyBox has no sleep applet; the
second used host-side delays and verified the actual USB shell transport.

This confirms cable communication, not a completed WebUSB installer. WebUSB
opening/claiming the ADB interface and browser protocol support still need a
separate test. The initial role switch currently requires the existing WLAN ADB
connection. A factory-new, USB-only installation flow remains unresolved.

## Factory-default question

The read-only `/etc/default.prop` on this device explicitly defaults
`persist.sys.usb.config=adb`, `sys.usb.config=adb`, `service.adb.tcp.port=5555`,
and `persist.sys.zkdebug=1`. This establishes that ADB support is part of the
manufacturer system. It does **not** establish the initial physical OTG role:
the observed role was host despite these ADB properties.

A search of Owlanzi's scripts, sources and platform code found no USB-role or
USB-mode-preference changes. The manufacturer's USB helper can restore a saved
mode, but the factory setting and kernel role selection have not been established.
The USB device-tree nodes inspected expose no obvious `dr_mode` property.
Testing a never-configured device is still needed to settle first-boot behavior;
do not infer it from the current device or perform a factory reset just to answer
this question.

## Initial findings on the device (before role switching)

- `/sys/devices/soc0/soc/soc:usbotg/otg_role` reports `usb_host`.
- `/etc/vold.fstab` enables the SStar EHCI USB host volume at `/mnt/usb1`.
  Its SD-card volume entry is commented out.
- No external USB volume was mounted during the inspection; the USB bus exposed
  its root hub only. USB-stick recognition has not yet been physically tested.
- The firmware also contains the SStar USB device controller and an ADB gadget:
  `functions=adb`, `enable=1`, VID/PID `18d1:d002`, and `sys.usb.config=adb`.
  Gadget state reports `CONFIGURED`, but the OTG role remains `usb_host`.
  These software values alone do not establish a working connection to a PC.
- The inspected Windows PC lists no corresponding USB device or USB ADB transport.
  Cable, physical port and connection to this PC have not yet been confirmed.
- `libzkhardware.so` exports `UsbSwitchHelper::setUsbMode` and references
  `sys_usb_mode_key` plus the OTG role/control paths. A device-mode experiment
  is therefore worth investigating, but is not a proven factory-state USB installer.
- The locally inspected manufacturer `libzkupgrade.so` contains `/mnt/usb1`,
  `/mnt/usb`, `zkautoupgrade`, `extupdate.img`, and upgrade functions. These are
  evidence of USB-aware upgrade support, not proof of automatic installation
  on this particular clock. The accepted image and trigger need validation.
- Internal partition `UDISK` is mounted read-only at `/mnt/storage`, containing
  `update.img` and `zkupgradetipbin`. This is internal storage, not an attached
  stick or evidence that Windows can access a mass-storage drive. Its image has
  not been validated as a complete recovery image or used in this investigation.

## Manufacturer documentation

- [TC002 FAQ](https://docs.ulanzistudio.com/tc002/en/faq/#what-is-the-port-on-the-left-side-of-tc002-used-for)
  explicitly says the left-side port supports USB-drive reading and charging.
- [Official FlyThings guide](https://github.com/UlanziTechnology/Ulanzi-U-Clock-TC002/blob/main/IDE%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E/%E8%AF%B4%E6%98%8E%E6%96%87%E6%A1%A3.md)
  describes temporary execution from external media and permanent `update.img`
  installation. Its generic SD-card instructions do not prove a physical SD slot
  exists on this TC002. It discourages USB debugging on Wi-Fi models, while also
  describing a USB-mode preference for some devices. Hardware verification takes
  precedence over extrapolating these generic instructions.

## Implications for Owlanzi

The strongest documented external-media option is a USB stick in the left-side
port. A website could distribute and explain preparation of that stick, but this
is different from direct browser flashing over a USB cable.

USB ADB is now confirmed after role switching, as described above. The browser
transport and a user-friendly way to enter that mode still need implementation.
Successful USB ADB transport does not solve persistent startup by itself.
The current Owlanzi OTA bundle is not a manufacturer `update.img`.

## Next physical checks

1. Completed: Windows enumeration and explicit USB ADB communication were
   verified without resetting or flashing the clock. Next, test browser WebUSB
   access and investigate entering device mode without prior WLAN onboarding.
2. Test a known FAT32 USB stick containing only a harmless marker file; no
   `update.img`, `extupdate.img`, `EasyUI.cfg`, or automatic-upgrade trigger files.
   Confirm mount and marker readability. Do not format any existing user drive.
3. Validate the exact image format and recovery contents before preparing a
   permanent Owlanzi image. Do not infer an accepted filename from strings alone.
4. Only then test installation, cold boot without external media, preservation
   of settings, and restoration to the manufacturer software.

Until those checks pass, USB installation and power-loss persistence must not be
advertised as verified capabilities.
