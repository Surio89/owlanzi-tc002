// SPDX-License-Identifier: GPL-3.0-or-later
// Isolate SDK's base::swap/template definitions from application C++ headers.
#define LOG_TAG "Owlanzi"
#include <base/wifi.h>
namespace owlanzi {
bool tc002_enable_wifi() { return base::wifiOnAndWait(10); }
}
