// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/update.hpp"
#include "owlanzi/version.hpp"
#include "owlanzi/time.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <stdexcept>
namespace owlanzi {
std::string updateManifestUrl(bool daily){
 return std::string("https://owlanzi.com/firmware/ota-tc002.json")+
  (daily?std::string("?daily-update-check=1&model=tc002&version=")+AppVersion:"");
}
namespace {
std::array<unsigned,3> parts(const std::string& s){std::array<unsigned,3> result{};std::size_t at=0;for(int i=0;i<3;++i){unsigned digits=0;while(at<s.size()&&s[at]>='0'&&s[at]<='9'){if(++digits>5)throw std::invalid_argument("Invalid version");result[i]=result[i]*10+s[at++]-'0';}if(!digits||(i<2&&(at>=s.size()||s[at++]!='.')))throw std::invalid_argument("Invalid version");}if(at!=s.size())throw std::invalid_argument("Invalid version");return result;}
std::int64_t day(const UpdateContext& c){if(c.utc<1700000000LL)return 0;return (c.utc+timeZoneOffset("Europe/Berlin",c.utc))/86400;}
}
bool newerVersion(const std::string& candidate,const std::string& current){try{return parts(candidate)>parts(current);}catch(...){return false;}}
Release parseRelease(const Json& j){
 if(j.at("schema")!=1||j.at("target")!="tc002"||j.at("kind")!="tc002-app-bundle"||j.at("abi")!="z21-stock-1"||j.at("loader")!=1)throw std::invalid_argument("Incompatible update");
 Release r;r.version=j.at("version").get<std::string>();parts(r.version);r.file=j.at("file").get<std::string>();r.sha256=j.at("sha256").get<std::string>();r.notes=j.value("notes",std::string{});
 const auto& size=j.at("size_bytes");if(!size.is_number_integer()||size<1024||size>4194304)throw std::invalid_argument("Invalid update size");r.size=size.get<std::size_t>();
 if(r.file!="owlanzi-tc002-"+r.version+"-ota.bin"||r.sha256.size()!=64||r.sha256.find_first_not_of("0123456789abcdef")!=std::string::npos||r.notes.size()>2000)throw std::invalid_argument("Invalid update metadata");return r;
}
struct UpdateService::Impl {
 std::unique_ptr<UpdateDriver> driver;std::string path;std::function<UpdateContext()> context;
 mutable std::mutex mutex;std::condition_variable wake;std::thread worker;std::atomic<bool> active{false};std::string command;std::int64_t lastDay=0;Release latest;
 Json state={{"current",AppVersion},{"latest",""},{"available",false},{"phase","idle"},{"error",""},{"received",0},{"size",0},{"notes",""}};
 Impl(std::unique_ptr<UpdateDriver> d,std::string directory):driver(std::move(d)),path(directory+"/update-check.json"){}
 void patch(Json j){std::lock_guard<std::mutex> lock(mutex);state.update(j);}
 void loop(){
  try{lastDay=Json::parse(readPrivateFile(path,1024)).value("day",std::int64_t{});}catch(...){}
  while(active){
   std::string action;{std::unique_lock<std::mutex> lock(mutex);wake.wait_for(lock,std::chrono::seconds(1),[&]{return !active||!command.empty();});action.swap(command);}if(!active)break;
   bool daily=false;
   try{
    patch(driver->inspect());
    const auto c=context();
    if(action.empty()&&c.daily&&c.online&&!c.critical&&c.uptimeMs>=60000&&day(c)>lastDay){
     lastDay=day(c);savePrivateFile(path,Json({{"day",lastDay}}).dump());action="check";daily=true;
    }
    if(action.empty())continue;
    if(!c.online||c.critical)throw std::runtime_error("Update unavailable during an alarm or without home Wi-Fi");
    patch({{"phase","checking"},{"error",""},{"received",0}});
    if(action=="rollback"){patch({{"phase","switching"}});driver->rollback();continue;}
    auto found=parseRelease(driver->fetchManifest(daily));latest=found;
    const bool available=newerVersion(found.version,AppVersion);
    patch({{"latest",found.version},{"available",available},{"notes",found.notes},{"size",found.size},{"phase",available?"available":"current"}});
    if(action!="install"||!available)continue;
    if(!driver->inspect().value("install_supported",false))throw std::runtime_error("OTA loader is not installed");
    patch({{"phase","downloading"}});
    driver->stage(found,[&](std::size_t n){patch({{"received",n}});},[&]{const auto now=context();return !active||now.critical||!now.online;});
    const auto ready=context();if(!active||ready.critical||!ready.online)throw std::runtime_error("Update interrupted before activation");
    patch({{"phase","switching"}});driver->activate(found.version);
   }catch(...){patch({{"phase","error"},{"error","update_failed"}});}
  }
 }
};
UpdateService::UpdateService(std::unique_ptr<UpdateDriver>d,std::string directory):impl_(std::make_unique<Impl>(std::move(d),std::move(directory))){}
UpdateService::~UpdateService(){stop();}
void UpdateService::start(std::function<UpdateContext()> context){if(impl_->active.exchange(true))return;impl_->context=std::move(context);impl_->state.update(impl_->driver->inspect());impl_->worker=std::thread([this]{impl_->loop();});}
void UpdateService::stop(){impl_->active=false;impl_->wake.notify_all();if(impl_->worker.joinable())impl_->worker.join();}
Json UpdateService::status()const{std::lock_guard<std::mutex> lock(impl_->mutex);return impl_->state;}
void UpdateService::request(const std::string& action){
 if(action!="check"&&action!="install"&&action!="rollback")throw std::invalid_argument("Unknown update action");
 const auto c=impl_->context();if(!c.online||c.critical)throw std::runtime_error("Update unavailable");
 std::lock_guard<std::mutex> lock(impl_->mutex);const auto phase=impl_->state.value("phase",std::string{});
 if(!impl_->active||!impl_->command.empty()||phase=="queued"||phase=="checking"||phase=="downloading"||phase=="switching")throw std::runtime_error("Update busy");
 impl_->command=action;impl_->state["phase"]="queued";impl_->state["error"]="";impl_->wake.notify_all();
}
}
