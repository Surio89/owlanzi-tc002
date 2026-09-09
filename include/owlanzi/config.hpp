// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "owlanzi/core.hpp"
#include "json.hpp"
#include <string>
namespace owlanzi {
using Json = nlohmann::json;
struct Config {
    CoreConfig core;
    std::string email, password, deviceSerial;
    bool europe = true;
};
Json configJson(const Config& config, bool includePassword = false);
Config mergeConfig(const Config& current, const Json& patch);
bool sameAccount(const Config& a, const Config& b);
std::string colorHex(std::uint32_t rgb);
// Atomic replacement. Linux credentials are owner-only; no private data logs.
void savePrivateFile(const std::string& path, const std::string& content);
std::string readPrivateFile(const std::string& path, std::size_t limit = 65536);
Config loadConfig(const std::string& directory);
void saveConfig(const std::string& directory, const Config& config);
std::string loadOrCreateToken(const std::string& directory);
}
