# Set up Wi-Fi directly in Owlanzi

Once Owlanzi is installed on the TC002, Wi-Fi setup no longer needs the
manufacturer interface. Existing Wi-Fi connections are reused. If no connection
is available, the application scans for networks and opens the **owlanzi** hotspot
after about 20 seconds. Setup requires no key or initial web password:

1. Connect your phone to `owlanzi` and stay connected even though it has no internet.
2. Open the setup page. If the phone does not open it automatically, visit
   `http://192.168.4.1` in a browser.
3. Under **System → 1 · Connect Wi-Fi**, select a network or enter its SSID, enter
   its password, and choose **Continue to Owlet account**. The hotspot stays open.
   Under **2 · Owlet account**, enter email, password, and region. Choose
   **Save Wi-Fi and Owlet · connect** to finish both steps.
4. Owlet credentials are saved before switching networks. Sign-in is attempted
   automatically after connecting to home Wi-Fi. The hotspot stops during the
   connection attempt. Reconnect your phone to the home network; the clock shows
   its new IP address on the matrix for 20 seconds.
5. Open that address. Select the desired Owlet device if several are available,
   or correct invalid Owlet credentials under System.

The TC002 web server is reachable on both ports 80 and 8080 on home Wi-Fi. All
interface files are stored on the clock, so hotspot setup needs no internet.

## Reconfiguration and error handling

**Set up Wi-Fi again** opens the hotspot for five minutes. Alternatively, hold
the middle button for six seconds and release it. Without further input, the
clock returns to its previous connection. On initial startup without a reachable
profile, the automatically opened hotspot has no timeout.

A connection attempt waits up to 45 seconds. If it fails, Owlanzi restores the
previous Wi-Fi profile and retries. If that profile is also unreachable, the
hotspot reopens after about another 20 seconds. An interrupted switch is also
rolled back on the next app start.

Critical alarms take priority over the Wi-Fi display and block manual network
switches. Resetting Owlanzi settings does not erase Wi-Fi. An optional web password
set later also protects the hotspot interface.

## Implementation and limitations

- `WifiService` runs the workflow in its own thread; HTTP and display rendering
  do not wait for Wi-Fi calls. Status and configuration APIs omit passwords.
- The native adapter uses the installed SDK `WifiManager` for Wi-Fi. Separate
  `hostapd` and `dnsmasq` processes provide the open AP, DHCP, and DNS. Application
  configuration has restrictive permissions under `/data/owlanzi`. System programs
  and manufacturer AP configuration are not replaced.
- The TC002 has one radio interface. No new scan is performed while the hotspot
  is active; its list comes from the scan immediately before switching. Manual
  SSID entry is always possible. Open networks and WPA/WPA2-Personal are supported;
  enterprise networks, WEP, and WPA3-only networks are not.
- Home Wi-Fi and hotspot operation alternate, briefly disconnecting the phone
  from the interface. Hotspot DNS resolves to `192.168.4.1`; unknown HTTP probe
  paths redirect there. Automatic opening depends on the phone.
- **Initial installation is separate:** The temporary ADB installer requires a
  clock already reachable on the LAN. A permanently installable package for
  factory-new devices without prior manufacturer setup still needs implementation
  and testing. This feature handles the app's Wi-Fi onboarding, not that initial
  transfer. It does not change owlanzi.com.

SDK references: [Wi-Fi](https://docs.flythings.cn/zh-hans/wifi.html) and
[hotspot](https://docs.flythings.cn/zh-hans/wifi_ap.html). The SDK hotspot manager
requires a password of at least eight characters and uses a different subnet,
so Owlanzi uses the existing system services directly.

## Verified on September 9, 2026

- Six CTest groups and eleven Node tests passed, as did the ARM build and checks
  against symbols exported by the actual device libraries.
- Simulation covered automatic hotspot without a profile, selection/connection,
  wrong-password rollback, cancellation, password redaction, and HTTP access control.
- Real TC002: existing Wi-Fi survived the app update; the interface worked on
  ports 80 and 8080, and scans returned networks. A hotspot opened for ten seconds
  returned automatically after about 21 seconds total, without an error.
- Still pending: a complete phone connection including DHCP/DNS and automatic
  opening, entry of a real new Wi-Fi password, the physical long press, initial
  startup without a profile, and power loss during a network switch on real hardware.

`tests/browser-wifi-check.js` exercises only a demo instance, covering scan
feedback, selection, password visibility, connection errors, and preservation
of Owlet and display settings.
