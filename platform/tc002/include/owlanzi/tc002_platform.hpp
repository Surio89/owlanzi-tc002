// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace owlanzi {

enum class InputEvent { Ack, BrightnessUp, BrightnessDown };

// One instance per FlyThings process. initialize(), ensure_wifi() and present()
// may block briefly and must be called by the application worker, never EasyUI.
class Tc002Platform {
public:
    static constexpr std::size_t width = 52;
    static constexpr std::size_t height = 16;
    static constexpr std::size_t frame_bytes = width * height * 3;

    Tc002Platform();
    ~Tc002Platform();
    Tc002Platform(const Tc002Platform&) = delete;
    Tc002Platform& operator=(const Tc002Platform&) = delete;

    bool initialize();
    void shutdown();
    // RGB888, row-major; values already include requested brightness.
    void present(const std::uint8_t* rgb, std::size_t size);
    std::vector<InputEvent> drain_inputs();
    bool ensure_wifi();
    void play_alarm(int volume);
    void stop_audio();
    const std::string& last_error() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Application runtime owns networking and render threads. Start must return
// promptly. Stop joins them before the EasyUI shared object can be unloaded.
void owlanzi_app_start(Tc002Platform& platform);
void owlanzi_app_stop();

} // namespace owlanzi
