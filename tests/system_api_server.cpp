// SPDX-License-Identifier: GPL-3.0-or-later
// Offline integration fixture: records system actions instead of touching hardware.
#include "owlanzi/runtime.hpp"
#include <chrono>
#include <fstream>
#include <thread>
#include <string>
#include <ctime>
int main(int argc,char** argv) {
 if(argc!=3)return 2;
 owlanzi::RuntimeOptions options;
 options.directory=argv[1];options.port=std::stoi(argv[2]);
 options.persistentBoot=true;
 // A fake time source records its use; it never changes the host system clock.
 options.networkTime=[directory=options.directory]{std::ofstream(directory+"/network-time.txt")<<"called";return static_cast<std::int64_t>(std::time(nullptr));};
 const auto receipt=options.directory+"/actions.txt";
 options.systemAction=[receipt](const std::string& action){std::ofstream(receipt,std::ios::app)<<action<<'\n';};
 owlanzi::Runtime runtime(options);runtime.start();
 std::this_thread::sleep_for(std::chrono::seconds(40));runtime.stop();
}
