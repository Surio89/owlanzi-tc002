// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/wifi.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <iostream>
using namespace owlanzi;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F>void until(F f){for(int i=0;i<300;++i){if(f())return;std::this_thread::sleep_for(std::chrono::milliseconds(5));}throw std::runtime_error("Wi-Fi state timeout");}
struct Driver:WifiDriver {
 std::atomic<int> commits{0},rollbacks{0},apStarts{0};bool configured=false,ap=false,pending=false,linked=false;
 std::string ssid;
 void enable()override{if(configured){linked=true;ssid="Home";}}
 void reconnect()override{enable();}
 bool hasSavedNetwork()override{return configured;}
 WifiLink inspect()override{return {true,linked,ap,linked?ssid:"",linked?"192.0.2.3":""};}
 std::vector<WifiNetwork> scan()override{return {{"Home",-50,true,true},{"Home",-70,true,true},{"Guest",-80,false,true}};}
 void startHotspot()override{ap=true;linked=false;++apStarts;}
 void stopHotspot()override{ap=false;}
 void connect(const std::string& name,const std::string& password)override{ap=false;pending=true;linked=password!="wrong-password";ssid=name;}
 void commit()override{configured=true;pending=false;++commits;}
 void rollback()override{if(pending){++rollbacks;pending=false;}ap=false;linked=configured;if(configured)ssid="Home";}
};
struct DelayedDriver:Driver {
 std::atomic<int> enables{0};int failures=2;
 void enable()override{if(++enables<=failures)throw std::runtime_error("radio still starting");Driver::enable();}
 WifiLink inspect()override{if(enables<=failures)return {};return Driver::inspect();}
};
struct FailedScanDriver:Driver {
 bool failHotspot=false;
 std::vector<WifiNetwork> scan()override{throw std::runtime_error("station scan unavailable");}
 void startHotspot()override{
  if(failHotspot){++apStarts;throw std::runtime_error("hotspot unavailable");}
  Driver::startHotspot();
 }
};
struct DormantDriver:Driver {
 std::atomic<int> reconnectCalls{0};std::atomic<bool> routerAvailable{true},dropLink{false};
 DormantDriver(){configured=true;}
 void enable()override{} // Radio is enabled, but the saved station is dormant.
 void rollback()override{ap=false;linked=false;}
 WifiLink inspect()override{if(dropLink.exchange(false))linked=false;return Driver::inspect();}
 void reconnect()override{++reconnectCalls;if(routerAvailable)Driver::enable();}
};
struct FailedHotspotDormantDriver:DormantDriver {
 void startHotspot()override{++apStarts;throw std::runtime_error("hotspot unavailable");}
};
}
int main(){try{
 check(hasSavedWifiNetwork("ctrl_interface=/data/misc/wifi/sockets\nnetwork={\n ssid=\"Home\"\n psk=\"private\"\n}\n"),"stock WLAN profile recognized without reading its password");
 check(hasSavedWifiNetwork(" network = {\n\tssid = 486f6d65\n}\n"),"hex SSID and whitespace recognized");
 for(const auto* empty:{"","# network={\n# ssid=\"Home\"\n","network={\nssid=\"\"\n}\n","ssid=\"outside profile\"\n","network={\n}\nssid=\"outside profile\"\n"})
  check(!hasSavedWifiNetwork(empty),"unconfigured clock keeps its setup hotspot open");
 {
  auto driver=std::make_unique<FailedHotspotDormantDriver>();auto* radio=driver.get();radio->routerAvailable=false;
  WifiService wifi(std::move(driver),{30,40,5,10,2,60});wifi.start();
  until([&]{return radio->apStarts>0&&wifi.status()["error"]=="wifi_operation_failed";});
  radio->routerAvailable=true;
  until([&]{return wifi.status()["phase"]=="connected";});
  check(!wifi.status()["hotspot"].get<bool>(),"a failed hotspot cannot prevent later station recovery");
  check(radio->commits==0&&radio->rollbacks==0,"failed fallback preserves saved credentials");wifi.stop();
 }
 {
  auto driver=std::make_unique<DormantDriver>();auto* radio=driver.get();
  WifiService wifi(std::move(driver),{30,40,5,10,2,60});wifi.start();
  until([&]{return wifi.status()["phase"]=="connected";});
  check(radio->reconnectCalls==1&&radio->apStarts==0,"enabled but dormant WLAN needs an active reconnect before hotspot fallback");
  radio->dropLink=true;
  until([&]{return radio->reconnectCalls==2&&wifi.status()["phase"]=="connected";});
  check(radio->commits==0&&radio->rollbacks==0,"reconnect never replaces the saved WLAN profile");wifi.stop();
 }
 {
  auto driver=std::make_unique<DormantDriver>();auto* radio=driver.get();radio->routerAvailable=false;
  WifiService wifi(std::move(driver),{30,40,5,10,2,60});wifi.start();
  until([&]{return wifi.status()["phase"]=="hotspot";});
  check(radio->reconnectCalls==2,"saved WLAN gets bounded active retries before setup fallback");
  radio->routerAvailable=true;
  until([&]{return wifi.status()["phase"]=="connected";});
  check(radio->reconnectCalls==3&&radio->apStarts==1,"router returning after cold boot is recovered without user intervention");
  check(radio->commits==0&&radio->rollbacks==0,"automatic fallback cycle preserves saved credentials");wifi.stop();
 }
 for(const bool manual:{false,true}){
  auto driver=std::make_unique<FailedScanDriver>();auto* radio=driver.get();radio->configured=manual;
  WifiService recovery(std::move(driver),{30,40,5,10});recovery.start();
  if(manual){until([&]{return recovery.status()["phase"]=="connected";});recovery.request("hotspot");}
  until([&]{return recovery.status()["hotspot"]==true;});
  check(recovery.status()["networks"].empty()&&!recovery.status()["scan_cached"].get<bool>(),"failed scan must not advertise cached networks");
  check(radio->commits==0&&radio->rollbacks==0,"opening recovery hotspot must not replace saved credentials");
  recovery.stop();
 }
 {
  auto driver=std::make_unique<FailedScanDriver>();auto* radio=driver.get();radio->failHotspot=true;
  WifiService recovery(std::move(driver),{30,40,5,10});recovery.start();
  until([&]{return radio->apStarts>0&&recovery.status()["error"]=="wifi_operation_failed";});
  check(!recovery.status()["hotspot"].get<bool>()&&!recovery.status()["last_hotspot_ok"].get<bool>(),"failed hotspot must not be announced as available");
  recovery.stop();
 }
 {
  auto delayed=std::make_unique<DelayedDriver>();auto* radio=delayed.get();radio->configured=true;
  WifiService boot(std::move(delayed),{40,40,5,10});boot.start();
  until([&]{return radio->enables==1;});
  std::this_thread::sleep_for(std::chrono::milliseconds(15));
  check(radio->enables==1,"startup retries are delayed, not a busy loop");
  until([&]{return boot.status()["phase"]=="connected";});
  check(radio->enables==3,"temporary radio startup failures are retried");
  check(radio->commits==0&&radio->apStarts==0,"startup recovery uses saved WLAN without replacing credentials or opening a hotspot");
  check(boot.status()["error"]=="","successful startup clears the temporary error");
  boot.stop();
 }
 auto driver=std::make_unique<Driver>();auto* d=driver.get();WifiService wifi(std::move(driver),{30,40,5,10});wifi.start();
 until([&]{return wifi.status()["hotspot"]==true;});
 check(wifi.status()["networks"].size()==2,"duplicate networks removed");
 check(wifi.status()["scan_cached"]==true,"hotspot scan is honestly marked cached");
 bool invalid=false;try{wifi.request("connect",{{"ssid","Home"},{"password","short"}});}catch(const std::invalid_argument&){invalid=true;}
 check(invalid,"bad password rejected before changing network");
 wifi.request("connect",{{"ssid","Home"},{"password","test-wifi-secret"}});
 until([&]{return d->commits==1&&wifi.status()["phase"]=="connected";});
 check(wifi.status().dump().find("test-wifi-secret")==std::string::npos,"password absent from public status");
 check(wifi.status()["show_address"]==true,"successful onboarding displays new address");
 wifi.request("connect",{{"ssid","Missing"},{"password","wrong-password"}});
 until([&]{return d->rollbacks==1&&wifi.status()["phase"]=="connected";});
 check(wifi.status()["ssid"]=="Home"&&wifi.status()["error"]=="wifi_join_failed","failed credentials restore working home profile");
 wifi.request("hotspot");until([&]{return wifi.status()["hotspot"]==true;});
 wifi.request("cancel");until([&]{return wifi.status()["phase"]=="connected";});
 check(wifi.status()["hotspot"]==false,"cancel closes setup hotspot");
 wifi.stop();check(!d->ap,"stop releases hotspot");
 std::cout<<"Wi-Fi onboarding, failure rollback, cached scanning and password redaction passed\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
