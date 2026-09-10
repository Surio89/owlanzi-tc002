// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/update.hpp"
#include "owlanzi/version.hpp"
#include <atomic>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
using namespace owlanzi;
namespace {
Json manifest(){return {{"schema",1},{"target","tc002"},{"kind","tc002-app-bundle"},{"abi","z21-stock-1"},{"loader",1},{"version","9.0.0"},{"file","owlanzi-tc002-9.0.0-ota.bin"},{"sha256",std::string(64,'a')},{"size_bytes",2048}};}
bool throws(std::function<void()> f){try{f();return false;}catch(...){return true;}}
void until(std::function<bool()> predicate){for(int i=0;i<400&&!predicate();++i)std::this_thread::sleep_for(std::chrono::milliseconds(10));assert(predicate());}
struct Fake:UpdateDriver{
 std::atomic<int> checks{0},daily{0},stages{0},installs{0},rollbacks{0};std::atomic<bool> fail{false},supported{true},interrupt{false};std::atomic<bool>* critical=nullptr;
 Json inspect()override{return {{"install_supported",supported.load()},{"rollback_available",true}};}
 Json fetchManifest(bool automatic)override{++checks;if(automatic)++daily;if(fail)throw std::runtime_error("offline");return manifest();}
 void stage(const Release&,std::function<void(std::size_t)> progress,std::function<bool()> cancel)override{++stages;progress(1024);if(interrupt&&critical)*critical=true;if(cancel())throw std::runtime_error("cancelled");progress(2048);}
 void activate(const std::string& v)override{assert(v=="9.0.0");++installs;}
 void rollback()override{++rollbacks;}
};
}
int main(){
 assert(updateManifestUrl(false)=="https://owlanzi.com/firmware/ota-tc002.json");
 assert(updateManifestUrl(true)==std::string("https://owlanzi.com/firmware/ota-tc002.json?daily-update-check=1&model=tc002&version=")+AppVersion);
 assert(newerVersion("1.10.0","1.9.99"));assert(!newerVersion("1.0.0","1.0.0"));assert(!newerVersion("1.0.0-beta","0.0.0"));assert(!newerVersion("1.0.0.1","0.0.0"));assert(!newerVersion("999999.0.0","0.0.0"));
 assert(parseRelease(manifest()).size==2048);
 for(const auto& field:{"schema","target","kind","abi","loader","version","file","sha256","size_bytes"}){auto bad=manifest();bad.erase(field);assert(throws([&]{parseRelease(bad);}));}
 for(const auto& patch:std::vector<Json>{{{"target","tc001"}},{{"abi","other"}},{{"file","https://example.com/a.bin"}},{{"file","../config.json"}},{{"sha256",std::string(64,'g')}},{{"size_bytes",-1}},{{"size_bytes",4194305}},{{"size_bytes",1024.5}},{{"version","9.0.0-beta"}}}){auto bad=manifest();bad.update(patch);assert(throws([&]{parseRelease(bad);}));}
 const auto path=std::filesystem::temp_directory_path()/std::filesystem::path("owlanzi-update-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(path);
 std::atomic<bool> critical{false},online{true},daily{true};std::atomic<std::int64_t> utc{1788955200};
 auto context=[&]{return UpdateContext{online,critical,daily,utc,60000};};
 {
  auto f=std::make_unique<Fake>();auto* driver=f.get();UpdateService service(std::move(f),path.string());service.start(context);
  until([&]{return service.status()["phase"]=="available";});assert(driver->daily==1&&driver->stages==0&&driver->installs==0);
  service.request("check");until([&]{return service.status()["phase"]=="available";});assert(driver->checks==2&&driver->daily==1);
  critical=true;assert(throws([&]{service.request("install");}));critical=false;
  online=false;assert(throws([&]{service.request("check");}));online=true;
  driver->supported=false;service.request("install");until([&]{return service.status()["phase"]=="error";});assert(driver->stages==0);
  driver->supported=true;driver->interrupt=true;driver->critical=&critical;service.request("install");until([&]{return service.status()["phase"]=="error";});assert(driver->stages==1&&driver->installs==0);critical=false;driver->interrupt=false;
  service.request("install");until([&]{return driver->installs==1;});assert(service.status()["received"]==2048);assert(throws([&]{service.request("install");}));service.stop();
 }
 {
  auto f=std::make_unique<Fake>();auto* driver=f.get();UpdateService service(std::move(f),path.string());service.start(context);
  std::this_thread::sleep_for(std::chrono::milliseconds(1200));assert(driver->checks==0);utc-=86400;std::this_thread::sleep_for(std::chrono::milliseconds(1200));assert(driver->checks==0);
  utc+=172800;driver->fail=true;until([&]{return service.status()["phase"]=="error";});assert(driver->daily==1);std::this_thread::sleep_for(std::chrono::milliseconds(1200));assert(driver->daily==1);service.stop();
 }
 {
  auto f=std::make_unique<Fake>();auto* driver=f.get();UpdateService service(std::move(f),path.string());daily=false;utc+=86400;service.start(context);
  std::this_thread::sleep_for(std::chrono::milliseconds(1200));assert(driver->checks==0);service.request("rollback");until([&]{return driver->rollbacks==1;});assert(driver->checks==0);service.stop();
 }
 Config c;c.password="private";c.core.brightness=8;auto changed=mergeConfig(c,{{"auto_update_check",false}});assert(!changed.autoUpdateCheck&&changed.password==c.password&&changed.core.brightness==8);assert(throws([&]{mergeConfig(c,{{"auto_update_check","false"}});}));
 std::filesystem::remove_all(path);std::cout<<"Update validation, daily scheduling, opt-out, alarms, cancellation and rollback passed\n";
}
