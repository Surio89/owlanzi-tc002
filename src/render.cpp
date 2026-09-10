// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/render.hpp"
#include "owlanzi/time.hpp"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace owlanzi {
namespace {

// Original 3x5 glyph drawings, expressed as five rows of three binary pixels.
// Unsupported characters deliberately render as '?' rather than disappear.
struct Glyph { char character; const char* rows; };
constexpr Glyph Font[] = {
    {' ', "000000000000000"}, {'-', "000000111000000"},
    {'.', "000000000000010"}, {':', "000010000010000"},
    {'?', "110001010000010"}, {'!', "010010010000010"},
    {'+', "000010111010000"}, {'/', "001001010100100"},
    {'%', "101001010100101"},
    {'0', "111101101101111"}, {'1', "010110010010111"},
    {'2', "110001111100111"}, {'3', "110001011001110"},
    {'4', "101101111001001"}, {'5', "111100110001110"},
    {'6', "011100111101111"}, {'7', "111001010010010"},
    {'8', "111101111101111"}, {'9', "111101111001110"},
    {'A', "010101111101101"}, {'B', "110101110101110"},
    {'C', "011100100100011"}, {'D', "110101101101110"},
    {'E', "111100110100111"}, {'F', "111100110100100"},
    {'G', "011100101101011"}, {'H', "101101111101101"},
    {'I', "111010010010111"}, {'J', "001001001101010"},
    {'K', "101101110101101"}, {'L', "100100100100111"},
    {'M', "101111111101101"}, {'N', "101111111111101"},
    {'O', "010101101101010"}, {'P', "110101110100100"},
    {'Q', "010101101011001"}, {'R', "110101110101101"},
    {'S', "011100010001110"}, {'T', "111010010010010"},
    {'U', "101101101101111"}, {'V', "101101101101010"},
    {'W', "101101111111101"}, {'X', "101101010101101"},
    {'Y', "101101010010010"}, {'Z', "111001010100111"},
};

void pixel(Frame& frame, int x, int y, std::uint32_t color) {
    if (x >= 0 && x < Frame::width && y >= 0 && y < Frame::height)
        frame.pixels[static_cast<std::size_t>(y * Frame::width + x)] = color & 0xffffff;
}

void rectangle(Frame& frame, int x, int y, int width, int height, std::uint32_t color) {
    for (int row = 0; row < height; ++row)
        for (int column = 0; column < width; ++column) pixel(frame, x + column, y + row, color);
}

int textWidth(std::string_view text) { return text.empty() ? 0 : static_cast<int>(text.size()) * 4 - 1; }

void text(Frame& frame, int x, int y, std::string_view value, std::uint32_t color) {
    for (char character : value) {
        if (character >= 'a' && character <= 'z') character -= 'a' - 'A';
        const char* rows = "110001010000010";
        for (const auto& glyph : Font) if (glyph.character == character) { rows = glyph.rows; break; }
        for (int row = 0; row < 5; ++row)
            for (int column = 0; column < 3; ++column)
                if (rows[row * 3 + column] == '1') pixel(frame, x + column, y + row, color);
        x += 4;
    }
}

void centered(Frame& frame, int y, std::string_view value, std::uint32_t color) {
    text(frame, (Frame::width - textWidth(value)) / 2, y, value, color);
}

void right(Frame& frame, int x, int y, std::string_view value, std::uint32_t color) {
    text(frame, x - textWidth(value) + 1, y, value, color);
}

void heart(Frame& frame, std::uint32_t color) {
    pixel(frame, 1, 1, color); pixel(frame, 3, 1, color);
    rectangle(frame, 0, 2, 5, 2, color);
    rectangle(frame, 1, 4, 3, 1, color); pixel(frame, 2, 5, color);
}

std::string rounded(float value) { return std::to_string(static_cast<int>(std::lround(value))); }

void battery(Frame& frame, int x, int y, int percent, bool charging, const Palette& palette) {
    rectangle(frame, x, y, 8, 1, palette.battery_frame);
    rectangle(frame, x, y + 4, 8, 1, palette.battery_frame);
    rectangle(frame, x, y + 1, 1, 3, palette.battery_frame);
    rectangle(frame, x + 7, y + 1, 1, 3, palette.battery_frame);
    pixel(frame, x + 8, y + 2, palette.battery_frame);
    const auto fill = charging ? palette.battery_charge : percent < 20 ? palette.battery_low : percent < 40 ? palette.battery_mid : palette.battery_fill;
    const int columns=percent>0?std::clamp(percent * 6 / 100,1,6):0;
    rectangle(frame, x + 1, y + 1, columns, 3, fill);
}

void scroll(Frame& frame, int y, std::string_view value, std::uint32_t color, Clock now) {
    const int width = textWidth(value);
    if (width <= Frame::width) { centered(frame, y, value, color); return; }
    // Begin with readable text and repeat with a full panel of separating space.
    const int period = width + Frame::width;
    const auto offset = static_cast<int>((now.monotonicMs / 60) % static_cast<std::uint64_t>(period));
    text(frame, -offset, y, value, color);
    text(frame, -offset + period, y, value, color);
}

void vitalsFrame(Frame& frame, const Vitals& vitals, const CoreConfig& config, Clock now) {
    const auto& p = config.palette;
    heart(frame, p.heart);
    right(frame, 18, 1, rounded(vitals.heart), p.numbers);
    text(frame, 27, 1, "O2", p.oxygen_label);
    right(frame, 51, 1, rounded(vitals.oxygen) + "%", p.oxygen);

    const char* label = "?";
    auto sleepColor = p.unknown_sleep;
    int barWidth = 2;
    switch (sleepState(vitals.sleepSt)) {
        case SleepState::Awake:
            label = config.german ? "WACH" : "AWAKE"; sleepColor = p.awake; barWidth = 20; break;
        case SleepState::Light:
            label = config.german ? "LEICHT" : "LIGHT"; sleepColor = p.light_sleep; barWidth = 10; break;
        case SleepState::Deep:
            label = config.german ? "TIEF" : "DEEP"; sleepColor = p.deep_sleep; barWidth = 4; break;
        default: break;
    }
    text(frame, 0, 9, label, sleepColor);
    rectangle(frame, (22 - barWidth) / 2, 15, barWidth, 1, sleepColor);
    right(frame, 51, 9, clockText(config.timeZone,now.displayUtcSeconds?now.displayUtcSeconds:now.utcSeconds), p.clock);
}

void waitingFrame(Frame& frame, const CoreConfig& config) {
    const auto& p = config.palette;
    heart(frame, p.heart_wait);
    right(frame, 18, 1, "--", p.waiting);
    text(frame, 27, 1, "O2", p.waiting);
    right(frame, 51, 1, "--%", p.waiting);
    centered(frame, 10, config.german ? "WARTE" : "WAITING", p.waiting_text);
}

void drawScreen(Frame& frame, Screen screen, const View& view, Clock now, bool preview) {
    const auto& config = view.config;
    const auto& p = config.palette;
    switch (screen) {
        case Screen::Vitals: {
            Vitals sample = view.vitals;
            if (preview) { sample.heart = 132; sample.oxygen = 97; sample.battery = 64; sample.sleepSt = 8; }
            vitalsFrame(frame, sample, config, now);
            break;
        }
        case Screen::Battery: {
            const int percent = preview ? 64 : static_cast<int>(std::lround(view.vitals.battery));
            const bool charging = preview || view.vitals.charging;
            centered(frame, 1, charging ? (config.german ? "LAEDT" : "CHARGING") :
                (config.german ? "SOCKE AUS" : "SOCK OFF"), charging ? p.charging_text : p.battery_status);
            battery(frame, 12, 9, percent, charging, p);
            right(frame, 40, 9, std::to_string(percent) + "%", p.battery);
            break;
        }
        case Screen::Waiting: waitingFrame(frame, config); break;
        case Screen::Offline:
            centered(frame, 1, "OFFLINE", p.offline);
            centered(frame, 10, config.german ? "VERBINDE" : "RETRYING", p.reconnect_text);
            break;
        case Screen::Setup:
            centered(frame, 1, "OWLANZI", p.setup_title);
            centered(frame, 10, "LOCAL SETUP", p.info);
            break;
        case Screen::Alarm: {
            const bool critical = preview || view.critical;
            const auto color = critical ? p.alarm : p.info;
            const auto value = preview ? std::string("TEST ALARM") : view.alarmText;
            scroll(frame, 2, value, color, now);
            centered(frame, 10, preview ? "PREVIEW" : view.silenced ?
                (config.german ? "STUMM" : "SILENCED") : !view.cloudFresh && critical ?
                "OFFLINE" : critical ? "OWLANZI" : (config.german ? "HINWEIS" : "NOTICE"), color);
            if (critical && ((now.monotonicMs / 500) & 1)) {
                pixel(frame, 0, 0, color); pixel(frame, 25, 0, color); pixel(frame, 51, 0, color);
            }
            break;
        }
        case Screen::Message:
            scroll(frame, 5, view.message, p.info, now);
            break;
        case Screen::Test:
            // Diagnostic corner colors verify orientation and RGB channel order.
            pixel(frame, 0, 0, 0xff0000); pixel(frame, 51, 0, 0x00ff00);
            pixel(frame, 0, 15, 0x0000ff); pixel(frame, 51, 15, 0xffffff);
            centered(frame, 5, "52 X 16", p.numbers);
            break;
    }
}

} // namespace

Frame renderFrame(const View& view, Clock now) {
    Frame frame;
    frame.brightness = view.brightness;
    if (view.screen == Screen::Test) drawScreen(frame, view.previewScreen, view, now, true);
    else drawScreen(frame, view.screen, view, now, false);
    return frame;
}

std::string frameHex(const Frame& frame) {
    constexpr char digits[] = "0123456789abcdef";
    std::string output;
    output.reserve(frame.pixels.size() * 6);
    for (const auto rgb : frame.pixels)
        for (int shift = 20; shift >= 0; shift -= 4) output += digits[(rgb >> shift) & 15];
    return output;
}

Frame renderWifiSetup(const CoreConfig& config,Clock now,bool hotspot,const std::string& address) {
    Frame frame;frame.brightness=static_cast<std::uint8_t>(std::max(config.brightness,config.previewBrightness));
    centered(frame,1,!address.empty()?"WLAN OK":hotspot?"WLAN SETUP":(config.german?"VERBINDE":"CONNECTING"),config.palette.setup_title);
    scroll(frame,10,!address.empty()?address:hotspot?"WLAN: OWLANZI  192.168.4.1":(config.german?"BITTE WARTEN":"PLEASE WAIT"),config.palette.info,now);
    return frame;
}

} // namespace owlanzi
