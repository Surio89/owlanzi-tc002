// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "owlanzi/config.hpp"
#include "owlanzi/render.hpp"
#include "owlanzi/wifi.hpp"
#include "owlanzi/update.hpp"
#include <memory>
#include <functional>
namespace owlanzi {
struct RuntimeOptions {
 bool demo=true;
 std::string directory=".local/demo",bind="127.0.0.1",caBundle;
 int port=8080,portalPort=0;
 std::shared_ptr<WifiService> wifi;
 std::shared_ptr<UpdateService> updates;
 std::function<std::int64_t()> networkTime;
 std::function<void(const std::string&)> systemAction;
 bool persistentBoot=false;
};
class Runtime {
public:
 explicit Runtime(RuntimeOptions options);
 ~Runtime();
 Runtime(const Runtime&)=delete;
 Runtime& operator=(const Runtime&)=delete;
 void start();void stop();
 Frame frame();
 bool consumeSound(int& volume);
 bool soundAllowed();
 void acknowledge();void adjustBrightness(int delta);
 Json status();Json config();void configure(const Json& patch);void demoScenario(const std::string& scenario);
 Json preview(const Json& options,bool activate);void stopPreview();void testSound(int volume);void stopSound();void resetSettings();
 bool running()const;
private:
 struct Impl;std::unique_ptr<Impl> impl_;
};
Clock currentClock();
}
