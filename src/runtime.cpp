// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/runtime.hpp"
#include "owlanzi/owlet.hpp"
#include "httplib.h"
#include "web_assets.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <stdexcept>
#include <ctime>
#include <algorithm>
namespace owlanzi {
Clock currentClock(){return {static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()),static_cast<std::int64_t>(std::time(nullptr))};}
namespace {
bool equalToken(const std::string& a,const std::string& b) {
 if(a.size()!=b.size())return false;unsigned difference=0;for(std::size_t i=0;i<a.size();++i)difference|=static_cast<unsigned char>(a[i])^static_cast<unsigned char>(b[i]);return difference==0;
}
std::string isoUtc(std::int64_t epoch) {
 std::time_t t=static_cast<std::time_t>(epoch);std::tm tm{};
#ifdef _WIN32
 gmtime_s(&tm,&t);
#else
 gmtime_r(&t,&tm);
#endif
 char out[32];std::strftime(out,sizeof(out),"%Y-%m-%dT%H:%M:%SZ",&tm);return out;
}
void jsonResponse(httplib::Response& response,const Json& json,int code=200) {response.status=code;response.set_content(json.dump(),"application/json; charset=utf-8");}
}
struct Runtime::Impl {
 RuntimeOptions options;Config configuration;Core core;std::string token;
 std::mutex mutex;std::condition_variable wake;
 std::atomic<bool> active{false};std::thread cloudThread,serverThread;
 httplib::Server server;
 std::uint64_t generation=0,pollCount=0,lastSuccessMs=0;std::int64_t lastSuccessUtc=0;
 std::string error,serial,scenario="vitals";std::vector<Device> devices;
 explicit Impl(RuntimeOptions o):options(std::move(o)),configuration(loadConfig(options.directory)),core(configuration.core),token(loadOrCreateToken(options.directory)) {
  if(options.port<1||options.port>65535)throw std::invalid_argument("Port out of range");
  core.setSetup(!options.demo&&(configuration.email.empty()||configuration.password.empty()));
 }
 void cloudLoop() {
  auto transport=options.demo?nullptr:makeHttpsTransport(options.caBundle,[this]{return !active.load();});
  auto client=transport?std::make_unique<OwletClient>(*transport):nullptr;
  while(active) {
   Config cfg;std::uint64_t g;std::string scene;
   {std::lock_guard<std::mutex> lock(mutex);cfg=configuration;g=generation;scene=scenario;}
   bool success=false;PollResult result;std::string failure;std::vector<Device> found;std::string chosen;
   if(options.demo) {
    if(scene!="offline"&&scene!="waiting") {
     auto now=currentClock();auto& v=result.vitals;v.valid=true;v.baseOn=true;v.sockConn=1;v.heart=124;v.oxygen=98;v.battery=76;v.sleepSt=15;v.measuredAt=now.utcSeconds;v.charging=scene=="charging";v.lowOx=scene=="alarm";if(v.lowOx)v.oxygen=84;
     result.appActive=true;success=true;found={{"DEMO-ONLY","Simulated Owlet"}};chosen="DEMO-ONLY";
    }else failure=scene=="offline"?"Demo: cloud unavailable":"Demo: waiting for a fresh measurement";
   }else if(cfg.email.empty()||cfg.password.empty()) failure="Enter your Owlet account in settings";
   else {
    try {client->configure(cfg);result=client->poll(currentClock());success=true;}
    catch(const std::exception& e){failure=e.what();}
    found=client->devices();chosen=client->serial();
   }
   std::unique_lock<std::mutex> lock(mutex);
   if(!active)break;
   if(g==generation) {
    auto now=currentClock();++pollCount;devices=found;serial=chosen;error=failure;
    if(success&&core.accept(result.vitals,result.appActive,now)){lastSuccessMs=now.monotonicMs;lastSuccessUtc=now.utcSeconds;if(!result.appActive)error="APP_ACTIVE failed; retrying activation";}
    else core.pollFailed(now);
   }
   auto seconds=options.demo?1:configuration.core.pollSeconds;
   // Back off failed login/network requests; never spin on invalid credentials.
   if(!success&&!options.demo)seconds=std::max(seconds,15);
   wake.wait_for(lock,std::chrono::seconds(seconds),[&]{return !active||generation!=g;});
  }
 }
};
Runtime::Runtime(RuntimeOptions options):impl_(std::make_unique<Impl>(std::move(options))){}
Runtime::~Runtime(){stop();}
const std::string& Runtime::pairingToken()const{return impl_->token;}
bool Runtime::running()const{return impl_->active.load();}
void Runtime::start() {
 auto& p=*impl_;if(p.active)return;
 p.server.set_payload_max_length(16384);p.server.set_read_timeout(5,0);p.server.set_write_timeout(5,0);
 p.server.set_keep_alive_max_count(20);
 p.server.new_task_queue=[] {return new httplib::ThreadPool(3,12);};
 p.server.set_pre_routing_handler([this](const httplib::Request& req,httplib::Response& res) {
  res.set_header("Cache-Control","no-store");res.set_header("X-Content-Type-Options","nosniff");
  res.set_header("Referrer-Policy","no-referrer");res.set_header("X-Frame-Options","DENY");
  res.set_header("Content-Security-Policy","default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' data:; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'self'");
  if(req.path.rfind("/api/",0)!=0)return httplib::Server::HandlerResponse::Unhandled;
  if(!equalToken(req.get_header_value("Authorization"),"Bearer "+impl_->token)) {jsonResponse(res,{{"error","Pair this browser with the local Owlanzi token"}},401);return httplib::Server::HandlerResponse::Handled;}
  if(req.method!="GET"&&req.method!="HEAD"&&(req.get_header_value("X-Owlanzi-Request")!="1"||req.get_header_value("Content-Type").rfind("application/json",0)!=0)) {
   jsonResponse(res,{{"error","JSON request header required"}},403);return httplib::Server::HandlerResponse::Handled;
  }
  return httplib::Server::HandlerResponse::Unhandled;
 });
 p.server.Get("/",[](const httplib::Request&,httplib::Response& r){r.set_content(WebIndex,"text/html; charset=utf-8");});
 p.server.Get("/app.js",[](const httplib::Request&,httplib::Response& r){r.set_content(WebScript,"text/javascript; charset=utf-8");});
 p.server.Get("/style.css",[](const httplib::Request&,httplib::Response& r){r.set_content(WebStyle,"text/css; charset=utf-8");});
 p.server.Get("/api/status",[this](const httplib::Request&,httplib::Response& r){jsonResponse(r,status());});
 p.server.Get("/api/config",[this](const httplib::Request&,httplib::Response& r){jsonResponse(r,config());});
 p.server.Post("/api/config",[this](const httplib::Request& req,httplib::Response& r){
  try {configure(Json::parse(req.body));jsonResponse(r,config());}
  catch(const std::invalid_argument& e){jsonResponse(r,{{"error",e.what()}},400);}
  catch(const Json::exception&){jsonResponse(r,{{"error","Invalid configuration JSON"}},400);}
  catch(...){jsonResponse(r,{{"error","Could not save configuration; existing settings retained"}},500);}
 });
 p.server.Post("/api/acknowledge",[this](const httplib::Request&,httplib::Response& r){acknowledge();jsonResponse(r,{{"ok",true}});});
 p.server.Post("/api/demo",[this](const httplib::Request& req,httplib::Response& r){
  if(!impl_->options.demo){jsonResponse(r,{{"error","Simulation is disabled on a live device"}},404);return;}
  try {demoScenario(Json::parse(req.body).at("scenario").get<std::string>());jsonResponse(r,{{"ok",true}});}
  catch(...){jsonResponse(r,{{"error","Invalid simulation scenario"}},400);}
 });
 p.server.set_exception_handler([](const httplib::Request&,httplib::Response& r,std::exception_ptr){jsonResponse(r,{{"error","Internal request error"}},500);});
 if(!p.server.bind_to_port(p.options.bind,p.options.port))throw std::runtime_error("Cannot bind local web interface (port in use?)");
 p.active=true;
 p.serverThread=std::thread([&p]{p.server.listen_after_bind();p.active=false;p.wake.notify_all();});
 p.cloudThread=std::thread([&p]{try{p.cloudLoop();}catch(...){std::lock_guard<std::mutex> lock(p.mutex);p.error="Cloud worker initialization failed";p.core.pollFailed(currentClock());}});
}
void Runtime::stop(){auto& p=*impl_;p.active=false;p.wake.notify_all();p.server.stop();if(p.cloudThread.joinable())p.cloudThread.join();if(p.serverThread.joinable())p.serverThread.join();}
Frame Runtime::frame(){auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);auto now=currentClock();p.core.tick(now);return renderFrame(p.core.view(now),now);}
bool Runtime::consumeSound(int& volume){auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);auto now=currentClock();p.core.tick(now);volume=p.configuration.core.volume;return p.core.consumeSound(now);}
bool Runtime::soundAllowed(){auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);auto now=currentClock();p.core.tick(now);auto v=p.core.view(now);return v.critical&&(v.alarmMask&~v.acknowledgedMask)&&p.configuration.core.soundEnabled&&p.configuration.core.volume>0;}
void Runtime::acknowledge(){auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);p.core.acknowledge();}
void Runtime::adjustBrightness(int delta){auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);Config cfg=p.configuration;cfg.core.brightness=std::clamp(cfg.core.brightness+delta,0,255);saveConfig(p.options.directory,cfg);p.configuration=cfg;p.core.configure(cfg.core,currentClock());}
Json Runtime::config(){auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);return configJson(p.configuration);}
void Runtime::configure(const Json& patch){
 auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);auto next=mergeConfig(p.configuration,patch);auto now=currentClock();
 saveConfig(p.options.directory,next);
 if(!sameAccount(next,p.configuration)){p.core.invalidate(now);p.devices.clear();p.serial.clear();p.lastSuccessMs=0;p.lastSuccessUtc=0;p.error.clear();}
 p.configuration=next;p.core.configure(next.core,now);p.core.setSetup(!p.options.demo&&(next.email.empty()||next.password.empty()));++p.generation;p.wake.notify_all();
}
void Runtime::demoScenario(const std::string& scene){
 auto& p=*impl_;if(!p.options.demo)throw std::invalid_argument("Simulation is disabled");
 if(scene!="vitals"&&scene!="charging"&&scene!="offline"&&scene!="alarm"&&scene!="waiting")throw std::invalid_argument("Unknown simulation scenario");
 std::lock_guard<std::mutex> lock(p.mutex);p.scenario=scene;
 if(scene=="offline")p.core.invalidate(currentClock());
 ++p.generation;p.wake.notify_all();
}
Json Runtime::status(){
 auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);auto now=currentClock();p.core.tick(now);auto view=p.core.view(now);auto frame=renderFrame(view,now);
 Json pixels=Json::array();for(auto pixel:frame.pixels)pixels.push_back(colorHex(pixel));
 Json devices=Json::array();for(const auto& d:p.devices)devices.push_back({{"serial",d.serial},{"name",d.name}});
 Json readings={{"heart_rate",nullptr},{"oxygen",nullptr},{"battery",nullptr},{"sleep_status",nullptr}};
 if(view.vitalsFresh){readings["heart_rate"]=view.vitals.heart;readings["oxygen"]=view.vitals.oxygen;
  switch(sleepState(view.vitals.sleepSt)) {case SleepState::Awake:readings["sleep_status"]="awake";break;case SleepState::Light:readings["sleep_status"]="light_sleep";break;case SleepState::Deep:readings["sleep_status"]="deep_sleep";break;default:readings["sleep_status"]="unknown_sleep";}
 }
 if(view.cloudFresh&&view.vitals.valid)readings["battery"]=view.vitals.battery;
 Json result={{"version","0.1.0-dev"},{"target","tc002"},{"mode",p.options.demo?"demo":"live"},{"hardware_verified",false},
 {"connected",view.cloudFresh},{"cloud_fresh",view.cloudFresh},{"vitals_fresh",view.vitalsFresh},{"state_message",view.reason},{"last_error",p.error},{"last_updated",nullptr},
 {"device",{{"serial",p.serial},{"name",p.options.demo?"Simulated Owlet":"Owlet"}}},{"devices",devices},{"readings",readings},
 {"display",{{"width",Frame::width},{"height",Frame::height},{"pixels",pixels},{"brightness",frame.brightness},{"screen",screenName(view.screen)}}},
 {"alarm",{{"active",!view.alarmText.empty()},{"critical",view.critical},{"message",view.alarmText},{"acknowledged",view.silenced}}},{"poll_count",p.pollCount}};
 if(p.lastSuccessUtc)result["last_updated"]=isoUtc(p.lastSuccessUtc);return result;
}
}
