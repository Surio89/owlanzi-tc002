// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/core.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace owlanzi;
namespace {
int checks = 0;
void expect(bool condition, const char* description) {
    ++checks;
    if (!condition) throw std::runtime_error(description);
}

struct Fixture {
    Clock now{0, parseUtc("2026-09-09T10:00:00Z")};
    Core core;
    Vitals value;
    explicit Fixture(bool own = false) {
        CoreConfig config;
        config.ownAlarms = own;
        core.configure(config, now);
        value.valid = true;
        value.baseOn = true;
        value.sockConn = 1;
        value.heart = 130;
        value.oxygen = 98;
        value.battery = 90;
        value.sleepSt = 8;
    }
    void advance(int seconds) { now.monotonicMs += seconds * 1000; now.utcSeconds += seconds; core.tick(now); }
    void fetch(bool updateStamp = true) {
        if (updateStamp) value.measuredAt = now.utcSeconds;
        expect(core.accept(value, true, now), "fixture sample accepted");
    }
    void fresh() { fetch(); advance(5); fetch(); }
    View view() const { return core.view(now); }
};

void timestampsAndValidation() {
    const auto stamp = parseUtc("2026-09-09T10:00:00Z");
    expect(stamp > 0, "valid UTC accepted");
    expect(stamp == parseUtc("2026-09-09T10:00:00.123Z"), "UTC fractional seconds");
    expect(stamp == parseUtc("2026-09-09T10:00:00+00:00"), "explicit UTC suffix");
    expect(parseUtc("2024-02-29T00:00:00Z") > 0, "leap day accepted");
    for (const auto* invalid : {"", "bad", "2026-02-29T00:00:00Z", "2026-04-31T00:00:00Z",
         "2026-09-09T10:00:00+02:00", "2026-09-09T10:00:00.Z", "2026-09-09T10:00:00Zextra",
         "2026-09-09T10:00:60Z", "2026-09-09T24:00:00Z", "2026-09-09T1x:00:00Z"})
        expect(!parseUtc(invalid), "invalid calendar or non-UTC rejected");
    expect(sleepState(0) == SleepState::Unknown && sleepState(2) == SleepState::Unknown,
        "unknown sleep is not guessed as awake");
    expect(sleepState(1) == SleepState::Awake && sleepState(8) == SleepState::Light &&
        sleepState(15) == SleepState::Deep, "existing sleep mapping retained");
    expect(std::string(sleepName(8, true)) == "leichter Schlaf", "German sleep description");
    CoreConfig config;
    expect(validCoreConfig(config) && !config.ownAlarms, "threshold rules off by default");
    config.hrLowLimit = 150; config.hrHighLimit = 100;
    expect(!validCoreConfig(config), "reversed pulse thresholds rejected");
    config = {}; config.spo2Seconds = 0;
    expect(!validCoreConfig(config), "zero duration rejected");
    config = {}; config.palette.heart = 0x1000000;
    expect(!validCoreConfig(config), "invalid palette color rejected");
    config = {}; config.volume = 7;
    expect(!validCoreConfig(config), "volume beyond device range rejected");
    Fixture f;
    f.value.heart = std::numeric_limits<float>::quiet_NaN();
    expect(!f.core.accept(f.value, true, f.now), "non-finite heart rate rejected");
    expect(f.view().screen == Screen::Offline, "invalid first fetch grants no grace");
}

void freshnessAndSessions() {
    Fixture f;
    f.fetch();
    expect(!f.view().vitalsFresh && f.view().screen == Screen::Waiting, "boot waits for session measurement");
    f.advance(5); f.fetch();
    expect(f.view().vitalsFresh && f.view().screen == Screen::Vitals, "new session measurement shown");
    f.advance(5); f.fetch();
    expect(f.view().vitalsFresh, "unchanged values with new timestamps stay current");
    f.value.measuredAt = f.now.utcSeconds + 1; f.fetch(false);
    expect(!f.view().vitalsFresh, "future measurement rejected");
    f.value.measuredAt = 0; f.fetch(false);
    expect(!f.view().vitalsFresh, "missing measurement timestamp rejected");
    f.fetch(); f.advance(61); f.fetch(false);
    expect(!f.view().vitalsFresh && f.view().cloudFresh, "cached cloud data does not reset measurement age");
    f.value.charging = 1; f.fetch();
    expect(f.view().screen == Screen::Battery, "charging shows battery");
    f.value.charging = 0; f.fetch(false);
    expect(!f.view().vitalsFresh, "charging exit rejects previous session data");
    f.advance(5); f.fetch(); expect(f.view().vitalsFresh, "charging exit accepts new measurement");
    f.value.sockOff = true; f.fetch();
    expect(f.view().screen == Screen::Battery, "removed sock shows battery");
    f.value.sockOff = false; f.value.baseOn = false; f.fetch();
    expect(!f.view().vitalsFresh, "base off hides readings");
    f.value.baseOn = true; f.value.sockConn = 0; f.fetch();
    expect(!f.view().vitalsFresh, "disconnected sock hides readings");
    f.value.sockConn = 1; f.fresh(); f.value.heart = 0; f.fetch();
    expect(f.view().screen == Screen::Waiting, "zero heart rate means unavailable");
    f.value.heart = 130; f.value.oxygen = 0; f.fetch();
    expect(f.view().screen == Screen::Waiting, "zero oxygen means unavailable");
    Fixture unset;
    unset.now.utcSeconds = 1; unset.fetch();
    expect(unset.view().sessionPending && !unset.view().vitalsFresh, "untrusted clock prevents session activation");
}

void offlineGrace() {
    Fixture f;
    expect(f.view().screen == Screen::Offline, "no offline grace before first fetch");
    f.fresh(); f.core.pollFailed(f.now);
    expect(f.view().screen == Screen::Waiting && !f.view().cloudFresh && !f.view().vitalsFresh,
        "failed poll hides values immediately during offline grace");
    f.advance(29); f.core.pollFailed(f.now);
    expect(f.view().screen == Screen::Waiting, "repeated failures do not remove bounded grace early");
    f.advance(1); f.core.pollFailed(f.now);
    expect(f.view().screen == Screen::Offline, "retries cannot postpone thirty-second offline deadline");
    f.fetch(); expect(f.view().screen == Screen::Vitals, "recovery clears offline immediately");
    f.core.setConnected(false, f.now);
    expect(f.view().screen == Screen::Waiting && !f.view().vitalsFresh, "network loss hides values immediately");
    f.core.setConnected(true, f.now);
    expect(!f.view().vitalsFresh, "network recovery alone does not restore old result");
    f.fetch(); f.advance(20);
    expect(f.view().cloudFresh, "fetch valid at exact twenty-second boundary");
    f.now.monotonicMs += 1; f.core.tick(f.now);
    expect(!f.view().cloudFresh && f.view().screen == Screen::Waiting, "stalled worker expires after twenty seconds");
    f.core.invalidate(f.now);
    expect(f.view().screen == Screen::Offline && !f.view().vitals.valid, "account change clears result and grace");
    Fixture zero;
    zero.fetch(); zero.core.pollFailed(zero.now);
    expect(zero.view().screen == Screen::Waiting, "successful fetch at monotonic zero grants grace");
    zero.now.utcSeconds += 100000;
    expect(zero.view().screen == Screen::Waiting, "wall-clock jump cannot shorten monotonic grace");
}

void ownAlarmDurations() {
    Fixture f(true);
    f.value.oxygen = 80; f.fresh();
    f.advance(5); f.fetch(); f.advance(5); f.fetch();
    expect(!f.view().critical, "threshold alarm waits full measured duration");
    f.advance(5); f.fetch();
    expect((f.view().alarmMask & 128) && f.view().critical, "threshold alarm after fifteen measured seconds");
    expect(f.view().alarmText.find("SUSTAINED") != std::string::npos, "own alarm visibly distinct from Owlet flags");
    CoreConfig disabled;
    expect(f.core.configure(disabled, f.now), "disable threshold alarm configuration accepted");
    expect(!f.view().critical, "disarming clears own alarm immediately");

    Fixture duplicate(true);
    duplicate.value.oxygen = 80; duplicate.fresh();
    duplicate.advance(15); duplicate.fetch(false);
    expect(!duplicate.view().critical, "duplicate measurement cannot advance alarm duration");
    duplicate.core.pollFailed(duplicate.now);
    duplicate.fetch();
    expect(!duplicate.view().critical, "failure resets own alarm duration");
    duplicate.advance(20); duplicate.fetch();
    expect(!duplicate.view().critical, "gap greater than twice poll plus five resets alarm duration");
    duplicate.advance(5); duplicate.fetch();
    duplicate.value.measuredAt -= 5; duplicate.fetch(false);
    expect(!duplicate.view().critical, "backward measurement resets duration");

    Fixture healthy(false);
    healthy.value.oxygen = 80; healthy.fresh();
    for (int index = 0; index < 4; ++index) { healthy.advance(5); healthy.fetch(); }
    expect(!healthy.view().critical, "low measurement alone does not enable opt-in rules");
}

void alarmsAndPrecedence() {
    Fixture f;
    f.core.setSetup(true);
    f.core.showMessage("IP 192.0.2.1", 10000, f.now);
    f.core.preview(Screen::Vitals, 10000, f.now);
    expect(f.view().screen == Screen::Test, "preview precedes message and setup");
    f.value.criticalOx = true; f.fetch();
    expect(f.view().screen == Screen::Alarm && f.view().critical, "real critical alarm interrupts previews and setup");
    expect(f.core.consumeSound(f.now), "first critical cause sounds immediately");
    expect(!f.core.consumeSound(f.now), "sound event consumed only once");
    f.core.acknowledge();
    expect(f.view().critical && f.view().silenced && f.view().brightness == 255, "ack retains display and alarm brightness");
    f.advance(25);
    expect(!f.core.consumeSound(f.now), "ack stops alarm repeats");
    f.value.highHr = true; f.fetch();
    expect(!f.view().silenced && f.core.consumeSound(f.now), "new cause re-arms sound after ack");
    f.core.acknowledge(); f.value.highHr = false; f.fetch(); f.value.highHr = true; f.fetch();
    expect(f.core.consumeSound(f.now), "ended and recurring cause re-arms sound");
    f.core.pollFailed(f.now);
    expect(f.view().screen == Screen::Alarm && f.view().alarmText.find("OFFLINE - LAST KNOWN") != std::string::npos,
        "last Owlet critical flag persists with offline annotation");
    f.advance(30);
    expect(f.view().screen == Screen::Alarm, "critical alarm outranks confirmed offline");
    f.core.preview(Screen::Vitals, 10000, f.now);
    expect(f.view().screen == Screen::Alarm, "critical alarm rejects later preview");
    f.value.criticalOx = false; f.value.highHr = false; f.fetch();
    expect(f.view().screen == Screen::Setup, "interrupted previews do not return after alarm ends");
    f.core.setSetup(false); f.value.criticalBatt = true; f.fetch();
    expect(f.view().screen == Screen::Alarm && !f.view().critical && f.view().brightness == 30,
        "critical battery remains quiet notice at normal brightness");
    expect(!f.core.consumeSound(f.now), "battery notice never sounds");
    f.advance(21);
    expect(f.view().screen == Screen::Waiting, "stale notice cannot hide waiting");
    f.advance(9);
    expect(f.view().screen == Screen::Offline, "stale notice cannot hide offline");
}

void repeatingSound() {
    Fixture f;
    f.value.lowHr = true; f.fetch();
    expect(f.core.consumeSound(f.now), "initial tone starts at monotonic zero");
    f.advance(24); expect(!f.core.consumeSound(f.now), "tone does not repeat early");
    f.advance(1); expect(f.core.consumeSound(f.now), "tone repeats at configured interval");
    auto config = f.view().config;
    config.alarmRepeatSeconds = 0; f.core.configure(config, f.now);
    f.advance(120); expect(!f.core.consumeSound(f.now), "repeat zero is once only");
    f.value.highHr = true; f.fetch();
    config.soundEnabled = false; f.core.configure(config, f.now);
    expect(!f.core.consumeSound(f.now), "sound disabled prevents output");
    config.soundEnabled = true; config.volume = 0; f.core.configure(config, f.now);
    expect(!f.core.consumeSound(f.now), "volume zero prevents output");
}

} // namespace

int main() {
    try {
        timestampsAndValidation(); freshnessAndSessions(); offlineGrace();
        ownAlarmDurations(); alarmsAndPrecedence(); repeatingSound();
        std::cout << "Core: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Core check failed: " << error.what() << '\n';
        return 1;
    }
}
