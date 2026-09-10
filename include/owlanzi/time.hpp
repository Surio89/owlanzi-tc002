// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace owlanzi {
bool validTimeZone(const std::string& name);
std::vector<std::string> timeZones();
int timeZoneOffset(const std::string& name,std::int64_t utc);
std::string localDateTime(const std::string& name,std::int64_t utc);
std::string clockText(const std::string& name,std::int64_t utc);
// Reject invalid calendar dates and nonexistent DST times. Repeated times use
// the first occurrence; callers display the resolved timestamp before saving.
std::int64_t localToUtc(const std::string& name,const std::string& local);
}
