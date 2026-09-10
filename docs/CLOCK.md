# Time on the TC002

In the readings view, **HH:MM** replaces the small battery in the lower right.
The clock occupies x=33 through 51 and y=9 through 13 and has its own `clock`
color. The large battery display while charging or when the sock is off remains
available. Sock battery is also still exposed as a status value in the web interface.

Under **System → Time**:

- **Automatic:** The TC002 queries `pool.ntp.org` over SNTP. After a successful
  synchronization it checks every six hours; after an error it waits at least
  one minute. Between checks, displayed time advances using the monotonic clock.
  Before the first synchronization it uses the existing device time. The web
  interface shows the time source and synchronization errors.
- **Time zone:** Defaults to Europe/Berlin. Choose from 598 IANA names or use the
  browser's time zone. Initial combined Wi-Fi/Owlet setup adopts the browser time
  zone when supported. Detection requires neither location permission nor an
  external service.
- **Manual:** Enter a date and time in the selected time zone, or copy the current
  phone time and save. Invalid dates and times skipped by the spring daylight
  saving transition are rejected. When a time occurs twice in autumn, the first
  occurrence is used.

Display time is separate from the system UTC used for TLS and Owlet measurement
age. Manual adjustments do not change measurement timestamps, alarm durations,
or freshness checks. While running, manual time advances monotonically. Time
anchors are saved for subsequent app starts; elapsed time between starts is
inferred from the device system clock. Retention of system time through a complete
power loss remains part of hardware acceptance.

Time zone rules come from [IANA tzdata 2026c](https://www.iana.org/time-zones).
Daylight saving transitions for 2020–2099 are embedded as compact shared tables.
No browser needs to remain open. Political rule changes require updated data in
a new app version. Regenerate with `python scripts/generate-timezones.py` and
`tzdata==2026.3`. Normal builds use the committed table and do not require that
Python package. Time calculations account for the TC002's 32-bit platform.

## Verification on September 9, 2026

Seven CTest groups and eleven Node tests passed. Coverage includes EU/US
transitions, half-hour offsets, day boundaries, 2050/2100 calendar boundaries,
invalid manual times, restored anchors, and color/pixel placement. API and mobile
browser checks confirmed manual/automatic settings and combined Wi-Fi/Owlet setup.

The ARM build was installed on the real TC002 and checked against its transfer
hash and device ABI. Automatic NTP synchronization with Europe/Berlin was confirmed;
Wi-Fi and Owlet were connected. Existing credentials, palette, and brightness were
preserved during the update. The physical matrix uses the same renderer as the
verified device API.
