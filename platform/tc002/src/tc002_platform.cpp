// SPDX-License-Identifier: GPL-3.0-or-later
// TC002 protocol and pin assignments derived from UlanziTechnology/
// Ulanzi-U-Clock-TC002, commit fa9d85d8e639430332c117cac71ce08dd6beb3f7.
// Original KeyManager input mapping: guoxs, 2022. Owlanzi adaptation: 2026.
#include "owlanzi/tc002_platform.hpp"

#include <mi_ao.h>
#include <utils/GpioHelper.h>
#include <utils/SpiHelper.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <deque>
#include <fcntl.h>
#include <linux/input.h>
#include <mutex>
#include <poll.h>
#include <stdexcept>
#include <termios.h>
#include <thread>
#include <unistd.h>

namespace owlanzi {
namespace {
constexpr std::size_t wire_row_bytes = 64 * 3;
constexpr int key_knob = 0x67, key_left = 0x6c, key_middle = 0x69, key_right = 0x6a;

// A successful version query enables the panel after each boot. This is the
// documented MCU transaction, with bounded parsing and unsigned checksums.
bool wake_panel(int fd) {
    termios config{};
    if (tcgetattr(fd, &config) < 0) return false;
    cfmakeraw(&config);
    cfsetispeed(&config, B1500000);
    cfsetospeed(&config, B1500000);
    config.c_cflag &= ~(CSTOPB | CRTSCTS);
    config.c_cflag |= CLOCAL | CREAD;
    if (tcsetattr(fd, TCSANOW, &config) < 0) return false;
    const std::array<std::uint8_t, 6> query{{0xff, 0x55, 0x11, 0x00, 0x01, 0x65}};
    for (int attempt = 0; attempt < 3; ++attempt) {
        tcflush(fd, TCIFLUSH);
        if (write(fd, query.data(), query.size()) != static_cast<ssize_t>(query.size())) continue;
        std::vector<std::uint8_t> received;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        while (std::chrono::steady_clock::now() < deadline) {
            pollfd port{fd, POLLIN, 0};
            if (poll(&port, 1, 50) <= 0) continue;
            std::uint8_t chunk[128];
            const auto count = read(fd, chunk, sizeof(chunk));
            if (count <= 0) continue;
            received.insert(received.end(), chunk, chunk + count);
            while (received.size() >= 6) {
                if (received[0] != 0xff || received[1] != 0x55 || received[3] > 32) {
                    received.erase(received.begin());
                    continue;
                }
                const std::size_t length = received[3] + 6;
                if (received.size() < length) break;
                std::uint16_t sum = 0;
                for (std::size_t i = 0; i < length - 2; ++i) sum += received[i];
                const auto expected = (static_cast<unsigned>(received[length - 2]) << 8) | received[length - 1];
                if (sum == expected && received[2] == 0x11 && received[3] > 0) return true;
                received.erase(received.begin(), received.begin() + length);
            }
        }
    }
    return false;
}
} // namespace

struct Tc002Platform::Impl {
    std::string error;
    int serial_fd = -1;
    std::array<int, 2> input_fd{{-1, -1}};
    std::unique_ptr<SpiHelper> spi;
    bool audio = false;
    std::mutex display_mutex, input_mutex, audio_mutex;
    std::atomic<bool> running{false};
    std::thread input_thread;
    std::deque<InputEvent> inputs;
    std::chrono::steady_clock::time_point previous_frame{};

    void enqueue(InputEvent event) {
        std::lock_guard<std::mutex> lock(input_mutex);
        if (inputs.size() < 32) inputs.push_back(event);
    }
    void read_inputs() {
        std::array<unsigned, 2> last_rotation{{0, 0}};
        std::chrono::steady_clock::time_point middlePressed{};
        while (running.load()) {
            pollfd ports[2]{{input_fd[0], POLLIN, 0}, {input_fd[1], POLLIN, 0}};
            if (poll(ports, 2, 100) <= 0) continue;
            for (std::size_t index = 0; index < 2; ++index) {
                if (!(ports[index].revents & POLLIN)) continue;
                input_event event{};
                while (read(input_fd[index], &event, sizeof(event)) == sizeof(event)) {
                    if (event.type == EV_ABS) {
                        if (event.value == 0x1 && last_rotation[index] == 0x8) enqueue(InputEvent::BrightnessUp);
                        if (event.value == 0xb && last_rotation[index] == 0xd) enqueue(InputEvent::BrightnessDown);
                        if (event.value == 0x8 || event.value == 0xd || event.value == 0x1 || event.value == 0xb)
                            last_rotation[index] = static_cast<unsigned>(event.value);
                    } else if (event.type == EV_KEY && event.code == key_middle && event.value == 0) {
                        if (middlePressed!=std::chrono::steady_clock::time_point{} && std::chrono::steady_clock::now()-middlePressed>=std::chrono::seconds(6)) enqueue(InputEvent::WifiSetup);
                        middlePressed={};
                    } else if (event.type == EV_KEY && event.value == 1) {
                        if (event.code == key_middle) middlePressed=std::chrono::steady_clock::now();
                        if (event.code == key_knob || event.code == key_middle) enqueue(InputEvent::Ack);
                        if (event.code == key_left) enqueue(InputEvent::BrightnessDown);
                        if (event.code == key_right) enqueue(InputEvent::BrightnessUp);
                    }
                }
            }
        }
    }
};

Tc002Platform::Tc002Platform() : impl_(new Impl) {}
Tc002Platform::~Tc002Platform() { shutdown(); }

bool Tc002Platform::initialize() {
    if (impl_->running.load()) return true;
    shutdown();
    // Ulanzi documents both auxiliary LEDs as active-high GPIO outputs.
    // Keep them dark even if the separate matrix handshake fails.
    for (const char* pin : {"GPIO_06", "GPIO_85"}) {
        if (GpioHelper::output(pin, 0) < 0)
            std::fprintf(stderr, "Owlanzi: could not switch off auxiliary LED %s.\n", pin);
    }
    impl_->serial_fd = open("/dev/ttyS1", O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (impl_->serial_fd < 0 || !wake_panel(impl_->serial_fd)) {
        impl_->error = "TC002 MCU version handshake failed on /dev/ttyS1";
        shutdown();
        return false;
    }
    impl_->spi.reset(new SpiHelper(0, SPI_MODE_0, 10 * 1000 * 1000, 8, false));
    impl_->input_fd[0] = open("/dev/input/event67", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    impl_->input_fd[1] = open("/dev/input/event68", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    // Missing inputs do not prevent the display/web setup from operating.
    impl_->running.store(true);
    impl_->input_thread = std::thread([this] { impl_->read_inputs(); });
    impl_->error.clear();
    return true;
}

void Tc002Platform::shutdown() {
    impl_->running.store(false);
    if (impl_->input_thread.joinable()) impl_->input_thread.join();
    stop_audio();
    for (auto& fd : impl_->input_fd) { if (fd >= 0) close(fd); fd = -1; }
    if (impl_->serial_fd >= 0) close(impl_->serial_fd);
    impl_->serial_fd = -1;
    impl_->spi.reset();
}

void Tc002Platform::present(const std::uint8_t* rgb, std::size_t size) {
    if (rgb == nullptr || size != frame_bytes) throw std::invalid_argument("TC002 frame must be 52x16 RGB888");
    std::lock_guard<std::mutex> lock(impl_->display_mutex);
    if (!impl_->spi) return;
    // The wire frame has 64 columns; the 12 unused columns must remain black.
    std::array<std::uint8_t, wire_row_bytes * height> wire{};
    for (std::size_t y = 0; y < height; ++y) std::memcpy(wire.data() + y * wire_row_bytes, rgb + y * width * 3, width * 3);
    std::this_thread::sleep_until(impl_->previous_frame + std::chrono::milliseconds(16));
    if (GpioHelper::output("GPIO_35", 0) < 0) throw std::runtime_error("TC002 panel sync GPIO unavailable");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    const bool sent = impl_->spi->write(wire.data(), wire.size());
    const bool released = GpioHelper::output("GPIO_35", 1) >= 0;
    impl_->previous_frame = std::chrono::steady_clock::now();
    if (!sent || !released) throw std::runtime_error("TC002 SPI frame write or sync failed");
}

std::vector<InputEvent> Tc002Platform::drain_inputs() {
    std::lock_guard<std::mutex> lock(impl_->input_mutex);
    std::vector<InputEvent> result(impl_->inputs.begin(), impl_->inputs.end());
    impl_->inputs.clear();
    return result;
}

void Tc002Platform::play_alarm(int volume) {
    std::lock_guard<std::mutex> lock(impl_->audio_mutex);
    static constexpr int levels[]{-60, -35, -31, -27, -23, -19, -15};
    volume = std::max(0, std::min(6, volume));
    if (volume == 0) { if (impl_->audio) MI_AO_ClearChnBuf(0,0); return; }
    // Use the clock's existing PCM output directly. No audio SDK static
    // binaries, decoders or FFmpeg code are embedded in the distributed app.
    if (!impl_->audio) {
        MI_AUDIO_Attr_t attr{};attr.eSamplerate=E_MI_AUDIO_SAMPLE_RATE_16000;attr.eBitwidth=E_MI_AUDIO_BIT_WIDTH_16;
        attr.eWorkmode=E_MI_AUDIO_MODE_I2S_MASTER;attr.eSoundmode=E_MI_AUDIO_SOUND_MODE_MONO;
        attr.u32FrmNum=12;attr.u32PtNumPerFrm=1024;attr.u32ChnCnt=1;
        if(MI_AO_SetPubAttr(0,&attr)!=0||MI_AO_Enable(0)!=0||MI_AO_EnableChn(0,0)!=0){MI_AO_Disable(0);throw std::runtime_error("TC002 audio output unavailable");}
        impl_->audio=true;
    }
    MI_AO_ClearChnBuf(0,0);MI_AO_SetVolume(0,0,levels[volume],E_MI_AO_GAIN_FADING_OFF);MI_AO_SetMute(0,0,FALSE);
    // Original synthesized double pulse; no third-party media asset is shipped.
    std::array<std::int16_t, 8000> samples{};
    for (std::size_t i = 0; i < samples.size(); ++i) {
        const auto phase = i % 4000;
        if (phase < 2400) {
            const double envelope = std::min(1.0, std::min(phase / 160.0, (2400 - phase) / 160.0));
            samples[i] = static_cast<std::int16_t>(7000 * envelope * std::sin(6.283185307179586 * 880 * i / 16000.0));
        }
    }
    for(std::size_t at=0;at<samples.size();at+=1024){MI_AUDIO_Frame_t frame{};frame.eBitwidth=E_MI_AUDIO_BIT_WIDTH_16;frame.eSoundmode=E_MI_AUDIO_SOUND_MODE_MONO;frame.apVirAddr[0]=samples.data()+at;frame.u32Len[0]=std::min<std::size_t>(1024,samples.size()-at)*sizeof(samples[0]);if(MI_AO_SendFrame(0,0,&frame,50)!=0){MI_AO_ClearChnBuf(0,0);throw std::runtime_error("TC002 audio transfer failed");}}
}

void Tc002Platform::stop_audio() {
    std::lock_guard<std::mutex> lock(impl_->audio_mutex);
    if (impl_->audio) { MI_AO_ClearChnBuf(0,0);MI_AO_SetMute(0,0,TRUE);MI_AO_DisableChn(0,0);MI_AO_Disable(0);impl_->audio=false; }
}

const std::string& Tc002Platform::last_error() const { return impl_->error; }
} // namespace owlanzi
