// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace owlanzi {

constexpr std::uint64_t FetchMaxMs = 20000;
constexpr std::uint64_t OfflineAfterMs = 30000;
constexpr std::int64_t MeasurementMaxSeconds = 60;

// Wall-clock UTC is used only for cloud measurement age. Durations/retries use
// a monotonic clock, so setting the device time cannot extend the offline grace.
struct Clock {
    std::uint64_t monotonicMs = 0;
    std::int64_t utcSeconds = 0;
};

enum class SleepState { Unknown = 0, Awake = 1, Light = 8, Deep = 15 };
SleepState sleepState(int raw);
const char* sleepName(int raw, bool german = false);
std::int64_t parseUtc(std::string_view value);

struct Vitals {
    bool valid = false;
    float oxygen = 0, heart = 0, battery = 0, oxygen10 = 0;
    bool baseOn = false;
    int sleepSt = 0, charging = 0, sockConn = 0, movement = 0;
    std::string hardware;
    bool lowOx = false, highOx = false, lowHr = false, highHr = false;
    bool lostPower = false, sockDiscon = false, sockOff = false;
    bool lowBatt = false, criticalOx = false, criticalBatt = false;
    std::int64_t measuredAt = 0;
};
bool validVitals(const Vitals& value);

struct Palette {
    std::uint32_t heart = 0x5E1220, numbers = 0x3E434C, separator = 0x1C1C1C;
    std::uint32_t waiting = 0x4A5260, awake = 0x7E5408;
    std::uint32_t light_sleep = 0x234E80, deep_sleep = 0x5E2A8C;
    std::uint32_t unknown_sleep = 0x242424, battery = 0x48505C;
    std::uint32_t alarm = 0xFF3030, info = 0xE3B341, offline = 0x8E2A2A;
};

struct CoreConfig {
    // Existing Owlanzi defaults. User-defined threshold alarms remain opt-in.
    bool ownAlarms = false;
    int spo2Limit = 86, spo2Seconds = 15;
    int hrLowLimit = 80, hrLowSeconds = 15;
    int hrHighLimit = 200, hrHighSeconds = 15;
    int pollSeconds = 5;
    bool german = false;
    int brightness = 30, alarmBrightness = 255;
    bool soundEnabled = true;
    int volume = 3, alarmRepeatSeconds = 25;
    Palette palette;
};
bool validCoreConfig(const CoreConfig& value);

enum class Screen { Alarm, Vitals, Battery, Waiting, Offline, Setup, Test, Message };
const char* screenName(Screen screen);

struct View {
    Screen screen = Screen::Offline;
    Vitals vitals;
    bool cloudFresh = false, vitalsFresh = false, offline = true;
    bool appActive = false, sessionPending = true;
    bool critical = false, silenced = false, soundPending = false;
    std::uint32_t alarmMask = 0, acknowledgedMask = 0;
    std::string alarmText;
    std::string message;
    std::string reason;
    Screen previewScreen = Screen::Vitals;
    std::uint8_t brightness = 30;
    CoreConfig config;
};

// The runtime owns synchronization. Core is deterministic and performs no I/O.
class Core {
public:
    explicit Core(CoreConfig config = {});
    bool configure(const CoreConfig& config, Clock now);
    bool accept(const Vitals& value, bool appActive, Clock now);
    void pollFailed(Clock now);
    void setConnected(bool connected, Clock now);
    void invalidate(Clock now);
    void tick(Clock now);
    void acknowledge();
    // Returns true once for a newly raised alarm, then at the configured repeat
    // interval. Acknowledgement stops further events but retains the display.
    bool consumeSound(Clock now);
    void setSetup(bool setup);
    void showMessage(std::string message, std::uint64_t durationMs, Clock now);
    void preview(Screen screen, std::uint64_t durationMs, Clock now);
    View view(Clock now) const;

private:
    bool cloudFresh(Clock now) const;
    bool vitalsFresh(Clock now) const;
    bool offline(Clock now) const;
    void resetOwn();
    void evaluate(bool newSample, Clock now);
    void recompute(Clock now);

    CoreConfig config_;
    Vitals vitals_;
    bool connected_ = true, cloudOk_ = false, haveSuccess_ = false;
    bool appActive_ = false, setup_ = false, sessionPending_ = true;
    std::uint64_t lastOkAt_ = 0;
    std::int64_t sessionAfter_ = 0, lastAlarmMeasurement_ = 0;
    std::int64_t spo2Since_ = 0, hrLowSince_ = 0, hrHighSince_ = 0;
    bool alSpo2_ = false, alHrLow_ = false, alHrHigh_ = false;
    std::uint32_t alarmMask_ = 0, acknowledgedMask_ = 0;
    bool soundPending_ = false, haveSound_ = false;
    std::uint64_t lastSoundAt_ = 0;
    std::string alarmText_, message_;
    bool messageActive_ = false, previewActive_ = false;
    std::uint64_t messageUntil_ = 0, previewUntil_ = 0;
    Screen previewScreen_ = Screen::Vitals;
};

} // namespace owlanzi
