// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "owlanzi/config.hpp"
#include "owlanzi/render.hpp"
#include <memory>
namespace owlanzi {
struct RuntimeOptions {
 bool demo=true;
 std::string directory=".local/demo",bind="127.0.0.1",caBundle;
 int port=8080;
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
 const std::string& pairingToken()const;
 bool running()const;
private:
 struct Impl;std::unique_ptr<Impl> impl_;
};
Clock currentClock();
}
