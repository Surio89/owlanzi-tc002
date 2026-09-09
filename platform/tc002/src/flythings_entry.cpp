// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/tc002_platform.hpp"

#include <entry/EasyUIContext.h>
#include <os/SystemProperties.h>

namespace {
owlanzi::Tc002Platform platform;
}

extern "C" {

void onEasyUIInit(EasyUIContext*) {
    // Vendor boot watchdog requires this before potentially slow initialization.
    SystemProperties::setString("sys.zkapp.state", "running");
    owlanzi::owlanzi_app_start(platform);
}

void onEasyUIDeinit(EasyUIContext*) {
    owlanzi::owlanzi_app_stop();
    platform.shutdown();
}

const char* onStartupApp(EasyUIContext*) {
    return "mainActivity";
}

}
