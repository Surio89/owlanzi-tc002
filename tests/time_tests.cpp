// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/time.hpp"
#include "owlanzi/config.hpp"
#include <cassert>
#include <iostream>
using namespace owlanzi;
int main(){
 const auto utc=[](const char* s){return parseUtc(s);};
 assert(clockText("Europe/Berlin",utc("2026-03-29T00:59:00Z"))=="01:59");
 assert(clockText("Europe/Berlin",utc("2026-03-29T01:00:00Z"))=="03:00");
 assert(clockText("Europe/Berlin",utc("2026-10-25T00:59:00Z"))=="02:59");
 assert(clockText("Europe/Berlin",utc("2026-10-25T01:00:00Z"))=="02:00");
 assert(clockText("America/New_York",utc("2026-03-08T07:00:00Z"))=="03:00");
 assert(clockText("Asia/Kolkata",utc("2026-09-09T20:00:00Z"))=="01:30");
 assert(localDateTime("Pacific/Auckland",utc("2099-12-31T12:00:00Z"))=="2100-01-01T01:00:00");
 assert(localDateTime("Europe/Berlin",utc("2050-07-01T12:34:00Z"))=="2050-07-01T14:34:00");
 assert(localToUtc("Europe/Berlin","2026-10-25T02:30")==utc("2026-10-25T00:30:00Z"));
 for(const char* invalid:{"2026-03-29T02:30","2026-02-30T12:00","2026-12-31T24:00","2026-09-09T12:61"}){
  bool rejected=false;try{localToUtc("Europe/Berlin",invalid);}catch(...){rejected=true;}assert(rejected);
 }
 Config cfg;const auto next=mergeConfig(cfg,{{"time",{{"zone","Europe/Berlin"},{"automatic",false},{"local","2026-09-09T21:37"}}}});
 const auto restored=mergeConfig({},configJson(next,true));
 assert(restored.core.manualUtc==next.core.manualUtc&&restored.core.manualSavedUtc==next.core.manualSavedUtc);
 assert(!configJson(next)["time"].contains("manual_utc"));
 std::cout<<"IANA timezone boundaries, 32-bit calendar range, manual validation and persistence passed\n";
}
