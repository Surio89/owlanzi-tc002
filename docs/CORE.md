# Portable core and display

The C++17 library in `include/owlanzi/core.hpp` and `src/core.cpp` owns the display
state, freshness gates and alarm rules. It performs no network, filesystem or
hardware operations. The runtime serializes calls and supplies a `Clock` with
monotonic milliseconds and Unix UTC seconds. Call `tick()` before observing a
frame or consuming an alarm sound event.

## Cloud result lifecycle

1. Deliver a fully parsed result through `accept(vitals, appActive, now)`.
   Non-finite and out-of-range measurements are rejected as failed polls.
2. Call `pollFailed(now)` after an unsuccessful fetch. A failed fetch hides all
   live measurements and battery status immediately. Reconnection alone cannot
   restore the previous result.
3. The maximum age of a successful fetch is 20 seconds. A failed or stalled fetch
   shows dashes until 30 seconds after the last success, then `OFFLINE`. There is
   no grace before the first successful result. Retry attempts do not extend it.
4. A displayed measurement also needs its own valid UTC timestamp, at most
   60 seconds old and never in the future. Re-fetching a cached measurement does
   not renew its age. An unchanged numeric value with a new timestamp is valid.
5. On boot, after charging/removal, or when the base/sock was inactive, wait for a
   measurement taken at least five seconds after observing a new active session.
   An untrusted device clock prevents this session gate from opening.
6. Use `invalidate(now)` after changing the account, region or selected device.
   This clears the previous values, alarm acknowledgement and offline grace.
   Network workers must separately reject old in-flight account results.

`APP_ACTIVE` success is exposed for diagnosis; a failed activation request does
not discard otherwise valid properties. The network layer is responsible for
renewing its session and retrying activation.

## Alarm behavior

Owlet low/high heart-rate, low/high oxygen, critical oxygen, disconnected sock
and base power-loss flags are critical display causes. These last known flags
remain visible during a cloud outage, with `OFFLINE - LAST KNOWN ALARM` appended
and unavailable numeric values replaced by `?`. A fresh result clears an ended
cause. Low/critical sock battery is a quiet notice, shown only while cloud data
is fresh.

User threshold alarms are off by default. When enabled, their duration advances
in measurement timestamp seconds, never in render ticks or fetch count. A failed
fetch, invalid/stale measurement, session transition, backward timestamp, or gap
greater than `2 * pollSeconds + 5` clears their duration. Updated threshold rules
start a new duration. `SUSTAINED` / `DAUERHAFT` identifies these own rules in the
alarm text.

`acknowledge()` acknowledges the current set of causes, stops pending/repeating
sound events, and retains the alarm display and configured alarm brightness.
A new cause or an ended cause that appears again sounds anew. `consumeSound()`
returns a one-time event for the initial sound and subsequent configured repeats;
zero repeat interval means once only. Sound disabled or volume zero yields no
sound event. Device audio playback must stop promptly when the returned view has
no unacknowledged alarm, even if a sequence is currently playing.

Owlanzi complements the Owlet base station. These rules are product display
behavior; they do not establish medical accuracy or replace the base alarm.

## Display and configuration

Screen order is critical alarm, active preview, message, setup, fresh quiet
notice, offline, waiting for reconnection, charging/removed-sock battery, current
vitals, and waiting for a valid measurement. A critical alarm cancels an active
preview or message so it does not return unexpectedly after the alarm ends.

The TC002 renderer uses a logical 52×16 RGB888 frame. Heart rate and oxygen share
the top row; sleep state and sock battery share the lower row. Known sleep states
retain the original mapping 1=awake, 8=light and 15=deep. Every other value is
shown as unknown with a neutral short bar. This mapping still needs comparison
against the Owlet app on the target account/device.

Brightness is a separate 0–255 value rather than baked into the pixels. Normal
and alarm brightness are user-controlled; no ambient sensor behavior is assumed.
`frameHex()` returns exactly 4,992 lowercase RGB hexadecimal characters in logical
row order, without an appended brightness byte. `preview(Screen::Test, ...)`
shows red/green/blue/white corners to verify panel orientation and channel order.
The 3×5 font glyphs in this repository are original row-based drawings.

`validCoreConfig()` validates the existing Owlanzi ranges: oxygen threshold
50–99, low pulse 30–150, high pulse 100–260 with low below high, durations 5–300
seconds, polling 5/10/15 seconds, brightness 0–255, volume 0–6, repeat 0–120 seconds,
and RGB palette colors 0–0xFFFFFF. Root application config handles credentials and
maps its JSON field names to this structure.

## Native regression checks

`tests/core_tests.cpp` exercises cloud freshness, session transitions, timestamp
validation, zero/missing values, the bounded offline grace, threshold durations,
acknowledgements, sound repeats and screen precedence. `tests/render_tests.cpp`
checks the logical frame dimensions, row-major encoding, color/orientation
diagnostic, sleep mapping, stale-value suppression and scrolling. These tests use
injected timestamps and fictional readings. They require no credentials or clock.
Hardware timing, actual panel output and audio playback need local TC002 testing.
