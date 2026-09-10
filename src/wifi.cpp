// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/wifi.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <stdexcept>
namespace owlanzi {
namespace {
using Time=std::chrono::steady_clock;
bool clean(const std::string& s){return s.find_first_of("\r\n") == std::string::npos&&s.find('\0')==std::string::npos;}
Json networksJson(std::vector<WifiNetwork> networks) {
 std::sort(networks.begin(),networks.end(),[](const auto&a,const auto&b){return a.rssi>b.rssi;});
 Json result=Json::array();std::vector<std::string> seen;
 for(const auto& n:networks)if(!n.ssid.empty()&&n.ssid.size()<=32&&clean(n.ssid)&&std::find(seen.begin(),seen.end(),n.ssid)==seen.end()){
  seen.push_back(n.ssid);result.push_back({{"ssid",n.ssid},{"rssi",n.rssi},{"secure",n.secure},{"supported",n.supported}});if(result.size()==32)break;
 }
 return result;
}
}
struct WifiService::Impl {
 std::unique_ptr<WifiDriver> driver;WifiTiming timing;
 mutable std::mutex mutex;std::condition_variable wake;std::thread thread;std::atomic<bool> active{false};
 std::string command,ssid,password;int hotspotSeconds=300;
 Json state={{"supported",true},{"phase","starting"},{"connected",false},{"ssid",""},{"ip",""},{"hotspot",false},{"hotspot_ssid","owlanzi"},{"hotspot_ip","192.168.4.1"},{"networks",Json::array()},{"scan_cached",false},{"error",""},{"last_hotspot_ok",false}};
 Impl(std::unique_ptr<WifiDriver> d,WifiTiming t):driver(std::move(d)),timing(t){}
 void update(const Json& patch){std::lock_guard<std::mutex> l(mutex);state.update(patch);}
 void run(){
  std::string phase="starting",target;Time::time_point deadline{},addressUntil{};bool transaction=false,wasConnected=false;
  auto transition=[&](const std::string& p,int ms=0){phase=p;deadline=ms?Time::now()+std::chrono::milliseconds(ms):Time::time_point{};update({{"phase",phase}});};
  auto hotspot=[&](int seconds){
   transition("opening_hotspot");update({{"networks",networksJson(driver->scan())},{"scan_cached",true}});
   driver->startHotspot();transition("hotspot",seconds*1000);update({{"last_hotspot_ok",true}});
  };
  try {driver->rollback();driver->enable();transition("reconnecting",timing.bootMs);}
  catch(...){update({{"error","wifi_start_failed"}});transition("error");}
  while(active){
   std::string action,name,pw;int seconds=300;
   {std::unique_lock<std::mutex> l(mutex);wake.wait_for(l,std::chrono::milliseconds(timing.pollMs),[&]{return !active||!command.empty();});if(!active)break;action.swap(command);name.swap(ssid);pw.swap(password);seconds=hotspotSeconds;}
   try {
    if(!action.empty()){
     // Give the HTTP response time to reach the browser before its WLAN drops.
     if(action!="scan"){std::unique_lock<std::mutex> l(mutex);if(wake.wait_for(l,std::chrono::milliseconds(timing.responseMs),[&]{return !active;}))break;}
     if(action=="connect"){
      update({{"error",""}});target=name;transaction=true;driver->connect(name,pw);std::fill(pw.begin(),pw.end(),'\0');pw.clear();transition("joining",timing.joinMs);
     }else if(action=="hotspot"){update({{"error",""}});hotspot(seconds);}
     else if(action=="cancel"){
      driver->stopHotspot();if(transaction){driver->rollback();transaction=false;}driver->enable();transition("reconnecting",timing.bootMs);
     }else if(action=="scan"){
      if(phase!="hotspot"){update({{"networks",networksJson(driver->scan())},{"scan_cached",false}});}
      update({{"scanning",false}});
     }
    }
    auto link=driver->inspect();
    update({{"supported",link.supported},{"connected",link.connected},{"ssid",link.ssid},{"ip",link.ip},{"hotspot",link.hotspot}});
    if(!link.supported){transition("unsupported");continue;}
    if(phase=="joining"){
     if(link.connected&&link.ssid==target&&!link.ip.empty()){
      driver->commit();transaction=false;wasConnected=true;transition("connected");addressUntil=Time::now()+std::chrono::seconds(20);update({{"error",""}});
     }else if(Time::now()>=deadline){driver->rollback();transaction=false;driver->enable();update({{"error","wifi_join_failed"}});transition("reconnecting",timing.bootMs);}
    }else if(phase=="hotspot"){
     if(!link.hotspot)throw std::runtime_error("Hotspot stopped");
     if(deadline!=Time::time_point{}&&Time::now()>=deadline){driver->stopHotspot();driver->enable();transition("reconnecting",timing.bootMs);}
    }else if(link.connected&&!link.ip.empty()){wasConnected=true;transition("connected");}
    else if(phase=="connected")transition("reconnecting",timing.bootMs);
    else if(phase=="reconnecting"&&Time::now()>=deadline)hotspot(wasConnected?300:0);
   }catch(...){
    std::fill(pw.begin(),pw.end(),'\0');
    try{driver->stopHotspot();if(transaction)driver->rollback();driver->enable();}catch(...){}
    transaction=false;update({{"error","wifi_operation_failed"},{"scanning",false}});transition("reconnecting",timing.bootMs);
   }
   {std::lock_guard<std::mutex> l(mutex);state["busy"]=phase=="joining"||phase=="opening_hotspot";state["show_address"]=phase=="connected"&&Time::now()<addressUntil;}
  }
  try{driver->stopHotspot();if(transaction)driver->rollback();}catch(...){}
 }
};
WifiService::WifiService(std::unique_ptr<WifiDriver> d,WifiTiming t):impl_(std::make_unique<Impl>(std::move(d),t)){}
WifiService::~WifiService(){stop();}
void WifiService::start(){if(impl_->active.exchange(true))return;impl_->thread=std::thread([this]{impl_->run();});}
void WifiService::stop(){impl_->active=false;impl_->wake.notify_all();if(impl_->thread.joinable())impl_->thread.join();}
Json WifiService::status()const{std::lock_guard<std::mutex> l(impl_->mutex);return impl_->state;}
void validateWifiConnect(const Json& input){
 if(!input.is_object())throw std::invalid_argument("Invalid Wi-Fi request");
 const auto ssid=input.at("ssid").get<std::string>(),password=input.value("password",std::string{});
 if(ssid.empty()||ssid.size()>32||!clean(ssid)||!clean(password)||(!password.empty()&&(password.size()<8||password.size()>63)))throw std::invalid_argument("Invalid Wi-Fi name or password");
}
void WifiService::request(const std::string& action,const Json& input){
 if(!input.is_object()||(action!="connect"&&action!="scan"&&action!="hotspot"&&action!="cancel"))throw std::invalid_argument("Invalid Wi-Fi request");
 std::string ssid,password;int seconds=300;
 if(action=="connect"){
  validateWifiConnect(input);
  ssid=input.at("ssid").get<std::string>();password=input.value("password",std::string{});
  if(ssid.empty()||ssid.size()>32||!clean(ssid)||!clean(password)||(!password.empty()&&(password.size()<8||password.size()>63)))throw std::invalid_argument("Invalid Wi-Fi name or password");
 }
 if(input.contains("duration_seconds")){
  if(!input["duration_seconds"].is_number_integer())throw std::invalid_argument("Invalid hotspot duration");
  const auto& duration=input["duration_seconds"];if(duration<10||duration>600)throw std::invalid_argument("Invalid hotspot duration");seconds=duration.get<int>();
 }
 std::lock_guard<std::mutex> l(impl_->mutex);
 if(!impl_->active||!impl_->state.value("supported",false))throw std::runtime_error("Wi-Fi unavailable");
 if(!impl_->command.empty()||(impl_->state.value("busy",false)&&action!="cancel"))throw std::runtime_error("Wi-Fi operation in progress");
 impl_->ssid=std::move(ssid);impl_->password=std::move(password);impl_->command=action;impl_->hotspotSeconds=seconds;
 if(action=="connect")impl_->state["phase"]="joining";
 if(action=="scan")impl_->state["scanning"]=true;
 if(action=="connect"||action=="hotspot")impl_->state["busy"]=true;
 impl_->wake.notify_all();
}
namespace {
class DemoWifi:public WifiDriver {
 WifiLink link{true,true,false,"Demo WLAN","192.0.2.2"};WifiLink previous=link;
public:
 void enable()override{}
 WifiLink inspect()override{return link;}
 std::vector<WifiNetwork> scan()override{return {{"Demo WLAN",-40,true,true},{"Guest WLAN",-65,false,true}};}
 void startHotspot()override{link={true,false,true,"",""};}
 void stopHotspot()override{if(link.hotspot)link=previous;}
 void connect(const std::string& name,const std::string& password)override{previous=link;link={true,password!="wrong-password",false,name,"192.0.2.2"};}
 void commit()override{previous=link;}
 void rollback()override{link=previous;}
};
}
std::shared_ptr<WifiService> makeDemoWifi(){return std::make_shared<WifiService>(std::make_unique<DemoWifi>(),WifiTiming{500,1500,50,100});}
}
