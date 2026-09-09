// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "owlanzi/core.hpp"
#include <array>
#include <string>

namespace owlanzi {

// Logical row-major RGB888, independent of the physical controller's format.
// Brightness remains separate so both the TC002 panel and browser can apply it.
struct Frame {
    static constexpr int width = 52;
    static constexpr int height = 16;
    std::array<std::uint32_t, width * height> pixels{};
    std::uint8_t brightness = 30;
};

Frame renderFrame(const View& view, Clock now);
std::string frameHex(const Frame& frame);

} // namespace owlanzi
