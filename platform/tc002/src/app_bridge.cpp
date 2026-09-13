// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/tc002_platform.hpp"
#include "owlanzi/runtime.hpp"
#include "owlanzi/version.hpp"

#include <manager/ConfigManager.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>
#include <unistd.h>
#include <sys/wait.h>
#include <stdexcept>

namespace owlanzi {
std::shared_ptr<WifiService> makeTc002Wifi(const std::string& directory);
std::int64_t tc002NetworkTime();
std::shared_ptr<UpdateService> makeTc002Updates(const std::string&,const std::string&,const std::string&);
void tc002UpdateHeartbeat(const std::string&);
namespace {
std::atomic<bool> active{false};
std::thread worker;

void run(Tc002Platform& platform) {
    std::fputs("Owlanzi: starting worker.\n", stderr);
    try {
        RuntimeOptions options;
        options.demo = false;
        options.bind = "0.0.0.0";
        options.port = 8080;
        options.portalPort = 80;
        const char* directory = std::getenv("OWLANZI_DATA_DIR");
        options.directory = directory && *directory ? directory : "/data/owlanzi";
        options.wifi=makeTc002Wifi(options.directory);
        options.networkTime=tc002NetworkTime;
        options.caBundle = CONFIGMANAGER->getResFilePath("cacert.pem");
        const auto startup=CONFIGMANAGER->getStartupLibPath();
        options.updates=makeTc002Updates(options.directory,options.caBundle,startup);
        options.persistentBoot=access("/res/bin/owlanzi-boot-control",X_OK)==0;
        if(options.persistentBoot)options.systemAction=[](const std::string& action){
            const char* arg=action=="restore-manufacturer"?"--stock":nullptr;
            if(!arg)throw std::invalid_argument("Unknown system action");
            auto p=fork();if(p<0)throw std::runtime_error("System action unavailable");
            if(p==0){for(int fd=3;fd<1024;++fd)close(fd);execl("/res/bin/owlanzi-boot-control","owlanzi-boot-control",arg,static_cast<char*>(nullptr));_exit(127);}
            int status=0;waitpid(p,&status,0);if(!WIFEXITED(status)||WEXITSTATUS(status))throw std::runtime_error("System action unavailable");
        };
        std::fputs("Owlanzi: initializing panel.\n", stderr);
        const bool panel_ready = platform.initialize();
        if (!panel_ready) std::fputs("Owlanzi: panel initialization failed; web diagnostics remain available.\n", stderr);
        std::fputs("Owlanzi: initializing runtime.\n", stderr);
        if (!active.load()) return;
        Runtime runtime(options);
        std::fputs("Owlanzi: starting HTTP service.\n", stderr);
        runtime.start();
        std::fputs("Owlanzi: rendering frames.\n", stderr);
        std::array<std::uint8_t, Tc002Platform::frame_bytes> rgb{};
        bool audio_started = false;
        bool panel_error_reported = false;
        const auto booted=std::chrono::steady_clock::now();auto nextHeartbeat=booted+std::chrono::seconds(10);
        while (active.load() && runtime.running()) {
            const auto next_frame = std::chrono::steady_clock::now() + std::chrono::milliseconds(33);
            for (const auto event : platform.drain_inputs()) {
                if (event == InputEvent::Ack) { runtime.acknowledge(); platform.stop_audio(); }
                if (event == InputEvent::BrightnessUp) runtime.adjustBrightness(5);
                if (event == InputEvent::BrightnessDown) runtime.adjustBrightness(-5);
                if (event == InputEvent::WifiSetup && !runtime.status()["alarm"]["critical"].get<bool>()) {
                    try { options.wifi->request("hotspot"); } catch (...) {}
                }
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
            if(panel_ready&&!panel_error_reported&&std::chrono::steady_clock::now()>=nextHeartbeat){tc002UpdateHeartbeat(startup);nextHeartbeat=std::chrono::steady_clock::now()+std::chrono::seconds(2);}
            int volume = 0;
            if (runtime.consumeSound(volume)) { try { platform.play_alarm(volume); audio_started = true; } catch(...) { std::fputs("Owlanzi: audio output failed.\n",stderr);platform.stop_audio(); } }
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
