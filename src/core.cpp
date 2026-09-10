// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/core.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace owlanzi {
namespace {

constexpr std::int64_t EarliestTrustedClock = 1700000000;

bool leapYear(int year) { return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0); }

bool between(int value, int minimum, int maximum) {
    return value >= minimum && value <= maximum;
}

bool elapsedWithin(std::uint64_t now, std::uint64_t before, std::uint64_t maximum) {
    return now >= before && now - before <= maximum;
}

void evaluateThreshold(bool below, std::int64_t& since, bool& flag,
                       int durationSeconds, std::int64_t measuredAt) {
    if (!below) { since = 0; flag = false; return; }
    if (!since) since = measuredAt;
    flag = measuredAt - since >= durationSeconds;
}

std::string rounded(float value) { return std::to_string(static_cast<int>(std::lround(value))); }

} // namespace

SleepState sleepState(int raw) {
    switch (raw) {
        case 1: return SleepState::Awake;
        case 8: return SleepState::Light;
        case 15: return SleepState::Deep;
        default: return SleepState::Unknown;
    }
}

const char* sleepName(int raw, bool german) {
    switch (sleepState(raw)) {
        case SleepState::Awake: return german ? "wach" : "awake";
        case SleepState::Light: return german ? "leichter Schlaf" : "light sleep";
        case SleepState::Deep: return german ? "Tiefschlaf" : "deep sleep";
        default: return german ? "unbekannt" : "unknown";
    }
}

std::int64_t parseUtc(std::string_view text) {
    if (text.size() < 20 || text[4] != '-' || text[7] != '-' || text[10] != 'T' ||
        text[13] != ':' || text[16] != ':') return 0;
    const auto number = [&](std::size_t offset, std::size_t length) {
        int value = 0;
        for (std::size_t index = offset; index < offset + length; ++index) {
            if (text[index] < '0' || text[index] > '9') return -1;
            value = value * 10 + text[index] - '0';
        }
        return value;
    };
    const int year = number(0, 4), month = number(5, 2), day = number(8, 2);
    const int hour = number(11, 2), minute = number(14, 2), second = number(17, 2);
    if (!between(year, 2023, 2100) || !between(month, 1, 12) || day < 1 ||
        !between(hour, 0, 23) || !between(minute, 0, 59) || !between(second, 0, 59)) return 0;
    std::size_t suffix = 19;
    if (text[suffix] == '.') {
        const auto start = ++suffix;
        while (suffix < text.size() && text[suffix] >= '0' && text[suffix] <= '9') ++suffix;
        if (start == suffix) return 0;
    }
    if (text.substr(suffix) != "Z" && text.substr(suffix) != "+00:00") return 0;
    constexpr int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (day > days[month - 1] + (month == 2 && leapYear(year))) return 0;
    std::int64_t total = 0;
    for (int candidate = 1970; candidate < year; ++candidate) total += 365 + leapYear(candidate);
    for (int candidate = 1; candidate < month; ++candidate)
        total += days[candidate - 1] + (candidate == 2 && leapYear(year));
    return (total + day - 1) * 86400 + hour * 3600 + minute * 60 + second;
}

bool validVitals(const Vitals& value) {
    return value.valid && std::isfinite(value.heart) && value.heart >= 0 && value.heart <= 400 &&
        std::isfinite(value.oxygen) && value.oxygen >= 0 && value.oxygen <= 100 &&
        std::isfinite(value.battery) && value.battery >= 0 && value.battery <= 100 &&
        std::isfinite(value.oxygen10) && value.oxygen10 >= 0 && value.oxygen10 <= 100 &&
        value.measuredAt >= 0;
}

bool validCoreConfig(const CoreConfig& value) {
    const auto& palette = value.palette;
    const std::uint32_t colors[] = {palette.heart, palette.numbers, palette.waiting,
        palette.awake, palette.light_sleep, palette.deep_sleep, palette.unknown_sleep, palette.battery,
        palette.alarm, palette.info, palette.offline, palette.heart_wait, palette.battery_frame,
        palette.battery_fill, palette.battery_charge, palette.battery_mid, palette.battery_low,
        palette.oxygen, palette.oxygen_label, palette.charging_text, palette.battery_status,
        palette.waiting_text, palette.reconnect_text, palette.setup_title};
    return between(value.spo2Limit, 50, 99) && between(value.spo2Seconds, 5, 300) &&
        between(value.hrLowLimit, 30, 150) && between(value.hrHighLimit, 100, 260) &&
        value.hrLowLimit < value.hrHighLimit && between(value.hrLowSeconds, 5, 300) &&
        between(value.hrHighSeconds, 5, 300) &&
        (value.pollSeconds == 5 || value.pollSeconds == 10 || value.pollSeconds == 15) &&
        between(value.brightness, 0, 255) && between(value.alarmBrightness, 0, 255) && between(value.previewBrightness, 0, 255) &&
        between(value.volume, 0, 6) && between(value.alarmRepeatSeconds, 0, 120) &&
        std::all_of(std::begin(colors), std::end(colors), [](std::uint32_t rgb) { return rgb <= 0xffffff; });
}

const char* screenName(Screen screen) {
    switch (screen) {
        case Screen::Alarm: return "alarm";
        case Screen::Vitals: return "vitals";
        case Screen::Battery: return "battery";
        case Screen::Waiting: return "waiting";
        case Screen::Offline: return "offline";
        case Screen::Setup: return "setup";
        case Screen::Message: return "message";
        case Screen::Test: return "test";
    }
    return "unknown";
}

Core::Core(CoreConfig config) : config_(std::move(config)) {
    if (!validCoreConfig(config_)) throw std::invalid_argument("Invalid Owlanzi core configuration");
}

bool Core::configure(const CoreConfig& config, Clock now) {
    if (!validCoreConfig(config)) return false;
    if (config.ownAlarms != config_.ownAlarms || config.spo2Limit != config_.spo2Limit ||
        config.spo2Seconds != config_.spo2Seconds || config.hrLowLimit != config_.hrLowLimit ||
        config.hrLowSeconds != config_.hrLowSeconds || config.hrHighLimit != config_.hrHighLimit ||
        config.hrHighSeconds != config_.hrHighSeconds || config.pollSeconds != config_.pollSeconds) resetOwn();
    config_ = config;
    evaluate(false, now);
    return true;
}

bool Core::cloudFresh(Clock now) const {
    return haveSuccess_ && cloudOk_ && connected_ && elapsedWithin(now.monotonicMs, lastOkAt_, FetchMaxMs);
}

bool Core::offline(Clock now) const {
    return !cloudFresh(now) && (!haveSuccess_ || now.monotonicMs < lastOkAt_ ||
        now.monotonicMs - lastOkAt_ >= OfflineAfterMs);
}

bool Core::vitalsFresh(Clock now) const {
    return cloudFresh(now) && vitals_.valid && vitals_.baseOn && vitals_.sockConn != 0 &&
        !vitals_.charging && !vitals_.sockOff && !sessionPending_ &&
        now.utcSeconds >= EarliestTrustedClock && vitals_.measuredAt >= sessionAfter_ &&
        vitals_.measuredAt <= now.utcSeconds && now.utcSeconds - vitals_.measuredAt <= MeasurementMaxSeconds &&
        vitals_.heart > 0 && vitals_.oxygen > 0;
}

bool Core::accept(const Vitals& value, bool appActive, Clock now) {
    if (!validVitals(value)) { pollFailed(now); return false; }
    if (value.charging || value.sockOff || !value.baseOn || !value.sockConn) {
        sessionPending_ = true;
        resetOwn();
    } else if (sessionPending_ && now.utcSeconds >= EarliestTrustedClock) {
        sessionAfter_ = now.utcSeconds + 5;
        sessionPending_ = false;
    }
    vitals_ = value;
    cloudOk_ = true;
    haveSuccess_ = true;
    lastOkAt_ = now.monotonicMs;
    appActive_ = appActive;
    evaluate(true, now);
    return true;
}

void Core::pollFailed(Clock now) {
    cloudOk_ = false;
    appActive_ = false;
    resetOwn();
    recompute(now);
}

void Core::setConnected(bool connected, Clock now) {
    if (!connected) cloudOk_ = false;
    connected_ = connected;
    evaluate(false, now);
}

void Core::invalidate(Clock now) {
    vitals_ = {};
    cloudOk_ = haveSuccess_ = appActive_ = false;
    lastOkAt_ = 0;
    sessionPending_ = true;
    sessionAfter_ = 0;
    resetOwn();
    recompute(now);
}

void Core::resetOwn() {
    spo2Since_ = hrLowSince_ = hrHighSince_ = lastAlarmMeasurement_ = 0;
    alSpo2_ = alHrLow_ = alHrHigh_ = false;
}

void Core::evaluate(bool newSample, Clock now) {
    if (!config_.ownAlarms || !vitalsFresh(now)) resetOwn();
    else if (newSample) {
        const auto stamp = vitals_.measuredAt;
        if (lastAlarmMeasurement_ && (stamp < lastAlarmMeasurement_ ||
            stamp - lastAlarmMeasurement_ > config_.pollSeconds * 2 + 5)) resetOwn();
        // Repeated fetches with the same timestamp never advance duration.
        // Values can be identical across distinct timestamps and remain valid.
        evaluateThreshold(vitals_.oxygen < config_.spo2Limit, spo2Since_, alSpo2_, config_.spo2Seconds, stamp);
        evaluateThreshold(vitals_.heart < config_.hrLowLimit, hrLowSince_, alHrLow_, config_.hrLowSeconds, stamp);
        evaluateThreshold(vitals_.heart > config_.hrHighLimit, hrHighSince_, alHrHigh_, config_.hrHighSeconds, stamp);
        lastAlarmMeasurement_ = stamp;
    }
    recompute(now);
}

void Core::recompute(Clock now) {
    const auto& v = vitals_;
    const std::uint32_t mask = (v.lowHr ? 1u : 0) | (v.highHr ? 2u : 0) |
        (v.lowOx ? 4u : 0) | (v.highOx ? 8u : 0) | (v.sockDiscon ? 16u : 0) |
        (v.lostPower ? 32u : 0) | (v.criticalOx ? 64u : 0) | (alSpo2_ ? 128u : 0) |
        (alHrLow_ ? 256u : 0) | (alHrHigh_ ? 512u : 0);
    acknowledgedMask_ &= mask;
    if (mask & ~alarmMask_ & ~acknowledgedMask_) soundPending_ = true;
    alarmMask_ = mask;
    if (!mask) { soundPending_ = false; haveSound_ = false; }
    else { previewActive_ = false; messageActive_ = false; }

    const auto language = [&](const char* english, const char* german) {
        return config_.german ? german : english;
    };
    const bool fresh = vitalsFresh(now);
    const std::string hr = fresh ? rounded(v.heart) : "?";
    const std::string ox = fresh ? rounded(v.oxygen) : "?";
    std::string text;
    const auto add = [&](const std::string& segment) {
        if (!text.empty()) text += "   +   ";
        text += segment;
    };
    if (v.lowHr) add(std::string(language("HEART RATE ", "PULS ")) + hr + language(" TOO LOW", " ZU NIEDRIG"));
    if (v.highHr) add(std::string(language("HEART RATE ", "PULS ")) + hr + language(" TOO HIGH", " ZU HOCH"));
    if (v.lowOx) add(std::string(language("OXYGEN ", "SAUERSTOFF ")) + ox + language(" TOO LOW", " ZU NIEDRIG"));
    if (v.highOx) add(std::string(language("OXYGEN ", "SAUERSTOFF ")) + ox + language(" TOO HIGH", " ZU HOCH"));
    if (v.criticalOx) add(language("OWLET CRITICAL OXYGEN ALERT", "OWLET KRITISCHER SAUERSTOFFALARM"));
    if (alSpo2_) add(std::string(language("OXYGEN ", "SAUERSTOFF ")) + ox +
        language(" SUSTAINED BELOW ", " DAUERHAFT UNTER ") + std::to_string(config_.spo2Limit));
    if (alHrLow_) add(std::string(language("HEART RATE ", "PULS ")) + hr +
        language(" SUSTAINED BELOW ", " DAUERHAFT UNTER ") + std::to_string(config_.hrLowLimit));
    if (alHrHigh_) add(std::string(language("HEART RATE ", "PULS ")) + hr +
        language(" SUSTAINED ABOVE ", " DAUERHAFT UEBER ") + std::to_string(config_.hrHighLimit));
    if (v.sockDiscon) add(language("SOCK DISCONNECTED", "SOCKE GETRENNT"));
    if (v.lostPower) add(language("BASE STATION LOST POWER", "BASISSTATION OHNE STROM"));
    if (cloudFresh(now)) {
        if (v.criticalBatt) add(language("SOCK BATTERY CRITICAL", "SOCKE AKKU KRITISCH"));
        else if (v.lowBatt) add(language("SOCK BATTERY LOW", "SOCKE AKKU LEER"));
    }
    if (mask && !cloudFresh(now)) add(language("OFFLINE - LAST KNOWN ALARM", "OFFLINE - LETZTER ALARM"));
    alarmText_ = std::move(text);
}

void Core::tick(Clock now) {
    if (messageActive_ && now.monotonicMs >= messageUntil_) messageActive_ = false;
    if (previewActive_ && now.monotonicMs >= previewUntil_) previewActive_ = false;
    evaluate(false, now);
}

void Core::acknowledge() {
    acknowledgedMask_ = alarmMask_;
    soundPending_ = false;
}

bool Core::consumeSound(Clock now) {
    if (!(alarmMask_ & ~acknowledgedMask_) || !config_.soundEnabled || config_.volume == 0) return false;
    const bool repeat = haveSound_ && config_.alarmRepeatSeconds && now.monotonicMs >= lastSoundAt_ &&
        now.monotonicMs - lastSoundAt_ >= static_cast<std::uint64_t>(config_.alarmRepeatSeconds) * 1000;
    if (!soundPending_ && !repeat) return false;
    soundPending_ = false;
    haveSound_ = true;
    lastSoundAt_ = now.monotonicMs;
    return true;
}

void Core::setSetup(bool setup) { setup_ = setup; }

void Core::showMessage(std::string message, std::uint64_t durationMs, Clock now) {
    if (alarmMask_) return;
    message_ = std::move(message);
    messageUntil_ = now.monotonicMs + std::min<std::uint64_t>(durationMs, 600000);
    messageActive_ = durationMs > 0;
}

void Core::preview(Screen screen, std::uint64_t durationMs, Clock now) {
    if (alarmMask_) return;
    previewScreen_ = screen;
    previewUntil_ = now.monotonicMs + std::min<std::uint64_t>(durationMs, 300000);
    previewActive_ = durationMs > 0;
}

View Core::view(Clock now) const {
    View result;
    result.config = config_;
    result.vitals = vitals_;
    result.cloudFresh = cloudFresh(now);
    result.vitalsFresh = vitalsFresh(now);
    result.offline = offline(now);
    result.appActive = appActive_;
    result.sessionPending = sessionPending_;
    result.alarmMask = alarmMask_;
    result.acknowledgedMask = acknowledgedMask_;
    result.critical = alarmMask_ != 0;
    result.silenced = alarmMask_ && !(alarmMask_ & ~acknowledgedMask_);
    result.soundPending = soundPending_;
    result.alarmText = alarmText_;
    result.message = message_;
    result.previewScreen = previewScreen_;
    result.brightness = static_cast<std::uint8_t>(config_.brightness);

    if (result.critical) {
        result.screen = Screen::Alarm;
        result.brightness = static_cast<std::uint8_t>(config_.alarmBrightness);
    } else if (previewActive_ && now.monotonicMs < previewUntil_) result.screen = Screen::Test;
    else if (messageActive_ && now.monotonicMs < messageUntil_) result.screen = Screen::Message;
    else if (setup_) result.screen = Screen::Setup;
    else if (!alarmText_.empty() && result.cloudFresh) result.screen = Screen::Alarm;
    else if (result.offline) result.screen = Screen::Offline;
    else if (!result.cloudFresh) result.screen = Screen::Waiting;
    else if (vitals_.charging || vitals_.sockOff) result.screen = Screen::Battery;
    else if (result.vitalsFresh) result.screen = Screen::Vitals;
    else result.screen = Screen::Waiting;

    const auto language = [&](const char* english, const char* german) {
        return config_.german ? german : english;
    };
    if (setup_) result.reason = language("Owlet account setup required", "Owlet-Konto einrichten");
    else if (result.offline) result.reason = language("Network or cloud fetch unavailable", "Netzwerk oder Cloud-Abruf nicht verfuegbar");
    else if (!result.cloudFresh) result.reason = language("Waiting for connection to recover", "Warte auf Wiederherstellung der Verbindung");
    else if (vitals_.charging) result.reason = language("Sock charging", "Socke laedt");
    else if (vitals_.sockOff) result.reason = language("Sock removed", "Socke abgenommen");
    else if (!vitals_.baseOn) result.reason = language("Base station inactive", "Basisstation inaktiv");
    else if (!vitals_.sockConn) result.reason = language("Sock not connected", "Socke nicht verbunden");
    else if (now.utcSeconds < EarliestTrustedClock) result.reason = language("Clock synchronization required", "Uhrzeit muss synchronisiert werden");
    else if (sessionPending_ || vitals_.measuredAt < sessionAfter_) result.reason = language("Waiting for a new session measurement", "Warte auf eine neue Messung dieser Sitzung");
    else if (!result.vitalsFresh) result.reason = language("Waiting for a current valid measurement", "Warte auf einen aktuellen gueltigen Messwert");
    else result.reason = language("Current measurement", "Aktuelle Messung");
    return result;
}

} // namespace owlanzi
