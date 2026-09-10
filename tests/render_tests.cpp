// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/render.hpp"
#include "owlanzi/time.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace owlanzi;
namespace {
int checks = 0;
void expect(bool condition, const char* description) {
    ++checks;
    if (!condition) throw std::runtime_error(description);
}
std::uint32_t pixel(const Frame& frame, int x, int y) {
    return frame.pixels[static_cast<std::size_t>(y * Frame::width + x)];
}
} // namespace

int main() {
    try {
        View view;
        view.screen = Screen::Vitals;
        view.vitals.heart = 132; view.vitals.oxygen = 97; view.vitals.battery = 64;
        view.vitals.sleepSt = 8; view.brightness = 17;
        const auto light = renderFrame(view, {});
        expect(light.pixels.size() == 832 && light.brightness == 17, "52x16 logical frame with separate brightness");
        expect(pixel(light, 1, 1) == view.config.palette.heart, "heart color preserved");
        expect(pixel(light, 6, 15) == view.config.palette.light_sleep && pixel(light, 5, 15) == 0,
            "light sleep bar has ten pixels");
        expect(pixel(light, 27, 9) == 0 && pixel(light, 25, 9) == 0 && pixel(light, 21, 3) == 0, "small battery and separator removed");
        view.config.palette.clock=0x123456;
        const auto timed=renderFrame(view,{0,parseUtc("2026-09-09T12:34:00Z")});
        expect(clockText("Europe/Berlin",parseUtc("2026-09-09T12:34:00Z"))=="14:34","local time includes daylight saving");
        bool timePixels=false;for(int y=0;y<16;y++)for(int x=0;x<52;x++)if(pixel(timed,x,y)==0x123456){timePixels=true;expect(x>=33&&y>=9&&y<=13,"clock color only affects bottom right");}
        expect(timePixels,"current time has separately colored pixels");
        view.vitals.battery=12;
        expect(timed.pixels==renderFrame(view,{0,parseUtc("2026-09-09T12:34:00Z")}).pixels,"battery percentage no longer changes readings layout");
        view.vitals.sleepSt = 2;
        const auto unknown = renderFrame(view, {});
        expect(pixel(unknown, 10, 15) == view.config.palette.unknown_sleep && pixel(unknown, 9, 15) == 0,
            "unknown sleep has short neutral bar");
        view.screen = Screen::Waiting;
        const auto waiting = renderFrame(view, {});
        view.vitals.heart = 299; view.vitals.oxygen = 80; view.vitals.battery = 1;
        expect(waiting.pixels == renderFrame(view, {}).pixels, "waiting never leaks cached numbers or battery");
        view.screen = Screen::Offline;
        const auto offline = renderFrame(view, {});
        view.vitals = {};
        expect(offline.pixels == renderFrame(view, {}).pixels, "offline never renders last measurements");
        view.screen = Screen::Test; view.previewScreen = Screen::Test;
        const auto corners = renderFrame(view, {});
        expect(pixel(corners, 0, 0) == 0xff0000 && pixel(corners, 51, 0) == 0x00ff00 &&
            pixel(corners, 0, 15) == 0x0000ff && pixel(corners, 51, 15) == 0xffffff,
            "diagnostic corners establish orientation and channel order");
        const auto hex = frameHex(corners);
        expect(hex.size() == 52 * 16 * 6 && hex.substr(0, 6) == "ff0000" &&
            hex.substr(hex.size() - 6) == "ffffff", "web frame encoding is fixed row-major RGB");
        view.screen = Screen::Alarm; view.critical = true;
        view.alarmText = "OWLET CRITICAL OXYGEN ALERT";
        const auto start = renderFrame(view, {});
        const auto later = renderFrame(view, {600, 0});
        expect(start.pixels != later.pixels, "alarm text scrolls without blocking");
        expect(pixel(later, 0, 0) == view.config.palette.alarm, "critical alarm corner blink");
        view.screen = Screen::Battery;
        view.vitals.battery = 0;
        const auto empty = renderFrame(view, {});
        expect(pixel(empty, 13, 10) == 0, "zero battery has empty fill instead of invented charge");
        view.vitals.battery = 15;
        const auto low = renderFrame(view, {});
        expect(pixel(low, 13, 10) == view.config.palette.battery_low && pixel(low, 14, 10) == 0,
            "low nonzero charge keeps one visible column in its own color");
        std::cout << "Renderer: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Renderer check failed: " << error.what() << '\n';
        return 1;
    }
}
