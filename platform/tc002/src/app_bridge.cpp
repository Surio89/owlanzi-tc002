// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/tc002_platform.hpp"
#include "owlanzi/runtime.hpp"

#include <manager/ConfigManager.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>

namespace owlanzi {
namespace {
std::atomic<bool> active{false};
std::thread worker;

void run(Tc002Platform& platform) {
    try {
        RuntimeOptions options;
        options.demo = false;
        options.bind = "0.0.0.0";
        options.port = 8080;
        const char* directory = std::getenv("OWLANZI_DATA_DIR");
        options.directory = directory && *directory ? directory : "/data/owlanzi";
        options.caBundle = CONFIGMANAGER->getResFilePath("cacert.pem");
        const bool panel_ready = platform.initialize();
        if (!panel_ready) std::fputs("Owlanzi: panel initialization failed; web diagnostics remain available.\n", stderr);
        platform.ensure_wifi();
        if (!active.load()) return;
        Runtime runtime(options);
        runtime.start();
        std::array<std::uint8_t, Tc002Platform::frame_bytes> rgb{};
        bool audio_started = false;
        bool panel_error_reported = false;
        while (active.load() && runtime.running()) {
            const auto next_frame = std::chrono::steady_clock::now() + std::chrono::milliseconds(33);
            for (const auto event : platform.drain_inputs()) {
                if (event == InputEvent::Ack) { runtime.acknowledge(); platform.stop_audio(); }
                if (event == InputEvent::BrightnessUp) runtime.adjustBrightness(5);
                if (event == InputEvent::BrightnessDown) runtime.adjustBrightness(-5);
            }
            const auto frame = runtime.frame();
            for (std::size_t i = 0; i < frame.pixels.size(); ++i) {
                for (unsigned channel = 0; channel < 3; ++channel) {
                    const unsigned value = (frame.pixels[i] >> (16 - channel * 8)) & 255;
                    rgb[i * 3 + channel] = static_cast<std::uint8_t>(value * frame.brightness / 255);
                }
            }
            if (panel_ready) {
                try { platform.present(rgb.data(), rgb.size()); panel_error_reported = false; }
                catch (...) {
                    if (!panel_error_reported) std::fputs("Owlanzi: matrix frame failed.\n", stderr);
                    panel_error_reported = true;
                }
            }
            int volume = 0;
            if (runtime.consumeSound(volume)) { platform.play_alarm(volume); audio_started = true; }
            if (audio_started && !runtime.soundAllowed()) { platform.stop_audio(); audio_started = false; }
            std::this_thread::sleep_until(next_frame);
        }
        runtime.stop();
    } catch (...) {
        // Never leak exception text, which might contain user data, into logcat.
        std::fputs("Owlanzi: application startup or worker failed.\n", stderr);
    }
    platform.shutdown();
    active.store(false);
}
}

void owlanzi_app_start(Tc002Platform& platform) {
    if (active.exchange(true)) return;
    if (worker.joinable()) worker.join();
    try { worker = std::thread([&platform] { run(platform); }); }
    catch (...) { active.store(false); std::fputs("Owlanzi: could not start application worker.\n", stderr); }
}

void owlanzi_app_stop() {
    active.store(false);
    if (worker.joinable()) worker.join();
}
}
