// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/tc002_input.hpp"
#include "owlanzi/runtime.hpp"
#include <cassert>
#include <filesystem>
#include <limits>
int main(int argc,char** argv){
 assert(argc==2);
 owlanzi::Tc002InputDecoder input;
 using Event=owlanzi::InputEvent;
 assert(!input.decode(3,0,1,0)); // No phantom turn from a partial event.
 assert(!input.decode(3,0,8,1));
 assert(!input.decode(0,0,0,1)); // Synchronization frame between phases.
 assert(input.decode(3,0,1,2)==Event::BrightnessUp);
 assert(!input.decode(3,0,1,3)); // End repeats must not brighten again.
 assert(!input.decode(3,0,13,4));
 assert(input.decode(3,0,11,5)==Event::BrightnessDown);
 assert(!input.decode(3,0,8,6));input.reset();
 assert(!input.decode(3,0,1,7));
 assert(input.decode(1,0x69,1,10)==Event::Ack);
 assert(!input.decode(1,0x69,0,6009));
 assert(input.decode(1,0x69,1,6010)==Event::Ack);
 assert(input.decode(1,0x69,0,12010)==Event::WifiSetup);
 assert(input.decode(1,0x6a,1,12011)==Event::BrightnessUp);
 assert(input.decode(1,0x6c,1,12012)==Event::BrightnessDown);
 assert(!input.decode(1,0x6a,2,12013));
 owlanzi::RuntimeOptions options;options.directory=argv[1];
 // No HTTP server, radio or cloud activity is started by this fixture.
 owlanzi::Runtime runtime(options);
 runtime.configure({{"brightness",100},{"preview_brightness",200}});
 runtime.preview({{"mode","colors"}},true);
 assert(runtime.frame().brightness==200);
 runtime.adjustBrightness(5);
 assert(runtime.frame().brightness==105);
 assert(runtime.config()["brightness"]==105);
 assert(!runtime.status()["preview_active"].get<bool>());
 runtime.adjustBrightness(std::numeric_limits<int>::max());assert(runtime.frame().brightness==255);
 runtime.adjustBrightness(std::numeric_limits<int>::min());assert(runtime.frame().brightness==0);
 runtime.adjustBrightness(5);assert(runtime.frame().brightness==5);
 owlanzi::Runtime reloaded(options);assert(reloaded.config()["brightness"]==5);
}
