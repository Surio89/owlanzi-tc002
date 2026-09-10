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
 WifiLink inspect()override{return {true,linked,ap,linked?ssid:"",linked?"192.0.2.3":""};}
 std::vector<WifiNetwork> scan()override{return {{"Home",-50,true,true},{"Home",-70,true,true},{"Guest",-80,false,true}};}
 void startHotspot()override{ap=true;linked=false;++apStarts;}
 void stopHotspot()override{ap=false;}
 void connect(const std::string& name,const std::string& password)override{ap=false;pending=true;linked=password!="wrong-password";ssid=name;}
 void commit()override{configured=true;pending=false;++commits;}
 void rollback()override{if(pending){++rollbacks;pending=false;}ap=false;linked=configured;if(configured)ssid="Home";}
};
}
int main(){try{
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
