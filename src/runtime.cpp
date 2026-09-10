// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/runtime.hpp"
#include "owlanzi/owlet.hpp"
#include "owlanzi/time.hpp"
#include "owlanzi/version.hpp"
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
Json displayJson(const Frame& frame,const std::string& screen) {
 Json pixels=Json::array();for(auto pixel:frame.pixels)pixels.push_back(colorHex(pixel));
 return {{"width",Frame::width},{"height",Frame::height},{"pixels",pixels},{"brightness",frame.brightness},{"screen",screen}};
}
Frame previewFrame(const Config& cfg,const std::string& mode,Clock now) {
 View v;v.config=cfg.core;v.brightness=static_cast<std::uint8_t>(cfg.core.previewBrightness);v.screen=Screen::Vitals;
 v.vitals.valid=true;v.vitals.heart=132;v.vitals.oxygen=97;v.vitals.battery=64;v.vitals.sleepSt=8;
 if(mode=="sleep1")v.vitals.sleepSt=1;else if(mode=="sleep3")v.vitals.sleepSt=15;else if(mode=="sleep0")v.vitals.sleepSt=0;
 if(mode=="battery"||mode=="charging"||mode=="battery-mid"||mode=="battery-low") {
  v.screen=Screen::Battery;v.vitals.charging=mode=="charging";
  v.vitals.battery=mode=="battery-mid"?35.f:mode=="battery-low"?15.f:64.f;
 }
 if(mode=="waiting")v.screen=Screen::Waiting;
 if(mode=="offline")v.screen=Screen::Offline;
 if(mode=="setup")v.screen=Screen::Setup;
 if(mode=="alarm"||mode=="info"){v.screen=Screen::Alarm;v.critical=mode=="alarm";v.cloudFresh=true;v.alarmText=mode=="alarm"?"TEST ALARM":"TEST INFO";}
 if(mode=="corners"){v.screen=Screen::Test;v.previewScreen=Screen::Test;}
 Frame frame=renderFrame(v,now);
 if(mode=="chase"||mode=="colors") {
  frame.pixels.fill(0);const std::uint32_t colors[]{0xff0000,0x00ff00,0x0000ff,0xffffff};
  if(mode=="chase")frame.pixels[(now.monotonicMs/50)%frame.pixels.size()]=0xffffff;
  else frame.pixels.fill(colors[(now.monotonicMs/1500)%4]);
 }
 return frame;
}
}
struct Runtime::Impl {
 RuntimeOptions options;Config configuration;Core core;
 Config previewConfig;std::string previewMode;std::uint64_t previewUntil=0,soundUntil=0;
 bool soundTestPending=false;int soundTestVolume=0;
 std::mutex mutex;std::condition_variable wake;
 std::atomic<bool> active{false};std::thread cloudThread,serverThread,portalThread,timeThread;
 httplib::Server server,portal;
 std::uint64_t generation=0,pollCount=0,failureCount=0,lastSuccessMs=0;std::int64_t lastSuccessUtc=0;
 const std::uint64_t startedAt=currentClock().monotonicMs;
 std::int64_t timeBaseUtc=currentClock().utcSeconds;
 std::uint64_t timeBaseMs=currentClock().monotonicMs,lastTimeSyncMs=0;
 bool timeSynced=false;std::string timeError;
 std::int64_t manualBaseUtc=0;std::uint64_t manualBaseMs=0;
 void anchorManual(){const auto now=currentClock();const auto& c=configuration.core;manualBaseUtc=c.manualUtc?c.manualUtc+now.utcSeconds-c.manualSavedUtc:now.utcSeconds;manualBaseMs=now.monotonicMs;}
 Clock displayClock(Clock now)const {now.displayUtcSeconds=configuration.core.automaticTime?timeBaseUtc+static_cast<std::int64_t>((now.monotonicMs-timeBaseMs)/1000):manualBaseUtc+static_cast<std::int64_t>((now.monotonicMs-manualBaseMs)/1000);return now;}
 void timeLoop(){
  while(active){
   bool needed;{std::lock_guard<std::mutex> lock(mutex);needed=configuration.core.automaticTime&&(!timeSynced||currentClock().monotonicMs-lastTimeSyncMs>=21600000);}
   if(needed){try{auto utc=options.networkTime();if(utc<1577836800LL||utc>=4102444800LL)throw std::runtime_error("Invalid network time");std::lock_guard<std::mutex> lock(mutex);timeBaseUtc=utc;timeBaseMs=lastTimeSyncMs=currentClock().monotonicMs;timeSynced=true;timeError.clear();}catch(...){std::lock_guard<std::mutex> lock(mutex);timeError="time_sync_failed";}}
   std::unique_lock<std::mutex> lock(mutex);wake.wait_for(lock,std::chrono::seconds(60),[&]{return !active;});
  }
 }
 std::string error,serial,scenario="vitals";std::vector<Device> devices;
 explicit Impl(RuntimeOptions o):options(std::move(o)),configuration(loadConfig(options.directory)),core(configuration.core) {
  if(options.port<1||options.port>65535)throw std::invalid_argument("Port out of range");
  core.setSetup(!options.demo&&(configuration.email.empty()||configuration.password.empty()));
  anchorManual();
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
    else {++failureCount;core.pollFailed(now);}
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
bool Runtime::running()const{return impl_->active.load();}
void Runtime::start() {
 auto& p=*impl_;if(p.active)return;
 const auto setupServer=[this,&p](httplib::Server& server){
 server.set_payload_max_length(16384);server.set_read_timeout(5,0);server.set_write_timeout(5,0);
 // Idle browser connections must not occupy the three device workers between
 // polls and prevent another phone from loading its settings.
 server.set_keep_alive_max_count(1);
 server.new_task_queue=[] {return new httplib::ThreadPool(3,12);};
 server.set_pre_routing_handler([](const httplib::Request&,httplib::Response& res) {
  res.set_header("Cache-Control","no-store");res.set_header("X-Content-Type-Options","nosniff");
  res.set_header("Referrer-Policy","no-referrer");res.set_header("X-Frame-Options","DENY");
  res.set_header("Content-Security-Policy","default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' data:; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'self'");
  return httplib::Server::HandlerResponse::Unhandled;
 });
 // Authenticate registered handlers after the bounded request body has been
 // read. Rejecting a POST before reading it can reset TCP before the browser
 // receives the JSON error (in particular when connections close promptly).
 const auto guarded=[this](httplib::Server::Handler handler) {
 return [this,handler](const httplib::Request& req,httplib::Response& res) {
  std::string password;{std::lock_guard<std::mutex> lock(impl_->mutex);password=impl_->configuration.webPassword;}
  if(!password.empty()&&!equalToken(req.get_header_value("Authorization"),httplib::make_basic_authentication_header("owlanzi",password).second)) {
   res.set_header("WWW-Authenticate","Basic realm=\"Owlanzi TC002\", charset=\"UTF-8\"");
   jsonResponse(res,{{"error","Web password required"}},401);return;
  }
  if(req.path.rfind("/api/",0)!=0){handler(req,res);return;}
  const auto origin=req.get_header_value("Origin"),host=req.get_header_value("Host");
  if((!origin.empty()&&origin!="http://"+host&&origin!="https://"+host)||req.get_header_value("Sec-Fetch-Site")=="cross-site") {
   jsonResponse(res,{{"error","Same-origin request required"}},403);return;
  }
  if(req.method!="GET"&&req.method!="HEAD"&&(req.get_header_value("X-Owlanzi-Request")!="1"||req.get_header_value("Content-Type").rfind("application/json",0)!=0)) {
   jsonResponse(res,{{"error","JSON request header required"}},403);return;
  }
  handler(req,res);
 };};
 const auto get=[&server,&guarded](const char* path,httplib::Server::Handler handler){server.Get(path,guarded(std::move(handler)));};
 const auto post=[&server,&guarded](const char* path,httplib::Server::Handler handler){server.Post(path,guarded(std::move(handler)));};
 get("/",[](const httplib::Request&,httplib::Response& r){r.set_content(WebIndex,"text/html; charset=utf-8");});
 get("/app.js",[](const httplib::Request&,httplib::Response& r){r.set_content(WebScript,"text/javascript; charset=utf-8");});
 get("/licenses.txt",[](const httplib::Request&,httplib::Response& r){r.set_content(WebLicenses,"text/plain; charset=utf-8");});
 get("/source.zip",[](const httplib::Request&,httplib::Response& r){r.set_redirect(std::string("https://owlanzi.com/firmware/owlanzi-tc002-")+AppVersion+"-source.zip");});
 get("/style.css",[](const httplib::Request&,httplib::Response& r){r.set_content(WebStyle,"text/css; charset=utf-8");});
 get("/api/status",[this](const httplib::Request&,httplib::Response& r){jsonResponse(r,status());});
 get("/api/config",[this](const httplib::Request&,httplib::Response& r){jsonResponse(r,config());});
 get("/api/update",[this](const httplib::Request&,httplib::Response& r){jsonResponse(r,impl_->options.updates?impl_->options.updates->status():Json({{"current",AppVersion},{"install_supported",false},{"phase","unavailable"}}));});
 post("/api/update/(check|install|rollback)",[this](const httplib::Request& req,httplib::Response& r){try{
  if(!impl_->options.updates){jsonResponse(r,{{"error","Updates unavailable"}},503);return;}
  const auto body=Json::parse(req.body);const std::string action=req.matches[1];
  if(action!="check"&&body.value("confirm",std::string{})!="UPDATE OWLANZI"){jsonResponse(r,{{"error","Confirm application restart"}},400);return;}
  impl_->options.updates->request(action);jsonResponse(r,{{"accepted",true}},202);
 }catch(const Json::exception&){jsonResponse(r,{{"error","Invalid update request"}},400);}catch(...){jsonResponse(r,{{"error","Update unavailable or busy"}},409);}});
 get("/api/timezones",[](const httplib::Request&,httplib::Response& r){jsonResponse(r,{{"version","2026c"},{"zones",timeZones()}});});
 post("/api/time/resolve",[](const httplib::Request& req,httplib::Response& r){try{const auto j=Json::parse(req.body);const auto zone=j.at("zone").get<std::string>();const auto utc=localToUtc(zone,j.at("local").get<std::string>());jsonResponse(r,{{"utc",utc},{"local",localDateTime(zone,utc)},{"offset_seconds",timeZoneOffset(zone,utc)}});}catch(...){jsonResponse(r,{{"error","Invalid local date/time; check daylight-saving transition"}},400);}});
 post("/api/setup",[this](const httplib::Request& req,httplib::Response& r){
  try{
   if(!impl_->options.wifi)throw std::invalid_argument("Wi-Fi unavailable");
   if(status()["alarm"]["critical"].get<bool>()){jsonResponse(r,{{"error","Alarm active; Wi-Fi change unavailable"}},409);return;}
   const auto body=Json::parse(req.body);const auto& wifi=body.at("wifi");validateWifiConnect(wifi);
   Json patch=Json::object();const auto& account=body.at("account");if(!account.is_object())throw std::invalid_argument("Invalid Owlet account");
   const auto wifiState=impl_->options.wifi->status();if(!wifiState.value("supported",false)||wifiState.value("busy",false))throw std::runtime_error("Wi-Fi unavailable");
   for(const char* key:{"email","password","region","device_serial","language","time"})if(account.contains(key))patch[key]=account.at(key);
   Config next;{std::lock_guard<std::mutex> lock(impl_->mutex);next=mergeConfig(impl_->configuration,patch);}
   if(next.email.empty()||next.email.find('@')==std::string::npos||next.password.empty())throw std::invalid_argument("Enter Owlet email and password");
   // Persist Owlet before queuing the WLAN change: the phone can leave the
   // hotspot without another request, even while the cloud is unavailable.
   configure(patch);impl_->options.wifi->request("connect",wifi);jsonResponse(r,{{"accepted",true}},202);
  }catch(const std::invalid_argument& e){jsonResponse(r,{{"error",e.what()}},400);}
   catch(const Json::exception&){jsonResponse(r,{{"error","Invalid setup settings"}},400);}
   catch(...){jsonResponse(r,{{"error","Setup could not complete; saved account settings are retained"}},409);}
 });
 post("/api/config",[this](const httplib::Request& req,httplib::Response& r){
  try {configure(Json::parse(req.body));jsonResponse(r,config());}
  catch(const std::invalid_argument& e){jsonResponse(r,{{"error",e.what()}},400);}
  catch(const Json::exception&){jsonResponse(r,{{"error","Invalid configuration JSON"}},400);}
  catch(...){jsonResponse(r,{{"error","Could not save configuration; existing settings retained"}},500);}
 });
 post("/api/acknowledge",[this](const httplib::Request&,httplib::Response& r){acknowledge();jsonResponse(r,{{"ok",true}});});
 for(const auto& route : {"/api/preview","/api/preview/render"})post(route,[this](const httplib::Request& req,httplib::Response& r){
  try{jsonResponse(r,preview(Json::parse(req.body),req.path=="/api/preview"));}
  catch(const std::invalid_argument&){jsonResponse(r,{{"error","Invalid preview settings"}},400);}
  catch(const Json::exception&){jsonResponse(r,{{"error","Invalid preview settings"}},400);}
  catch(const std::runtime_error& e){jsonResponse(r,{{"error",e.what()}},409);}
 });
 post("/api/preview/stop",[this](const httplib::Request&,httplib::Response& r){stopPreview();jsonResponse(r,{{"ok",true}});});
 get("/api/defaults",[](const httplib::Request&,httplib::Response& r){jsonResponse(r,configJson(Config{}));});
 post("/api/sound",[this](const httplib::Request& req,httplib::Response& r){
  try{auto body=Json::parse(req.body);auto cfg=mergeConfig(Config{},{{"alarms",{{"volume",body.at("volume")}}}});testSound(cfg.core.volume);jsonResponse(r,{{"ok",true}});}
  catch(...){jsonResponse(r,{{"error","Invalid volume or active alarm"}},400);}
 });
 post("/api/sound/stop",[this](const httplib::Request&,httplib::Response& r){stopSound();jsonResponse(r,{{"ok",true}});});
 post("/api/reset",[this](const httplib::Request& req,httplib::Response& r){
  try{if(Json::parse(req.body).value("confirm",std::string{})!="RESET OWLANZI")throw std::invalid_argument("Confirmation required");resetSettings();jsonResponse(r,{{"ok",true}});}
  catch(...){jsonResponse(r,{{"error","Reset failed or confirmation missing"}},400);}
 });
 post("/api/demo",[this](const httplib::Request& req,httplib::Response& r){
  if(!impl_->options.demo){jsonResponse(r,{{"error","Simulation is disabled on a live device"}},404);return;}
  try {demoScenario(Json::parse(req.body).at("scenario").get<std::string>());jsonResponse(r,{{"ok",true}});}
  catch(...){jsonResponse(r,{{"error","Invalid simulation scenario"}},400);}
 });
 get("/api/wifi",[this](const httplib::Request&,httplib::Response& r){jsonResponse(r,impl_->options.wifi?impl_->options.wifi->status():Json({{"supported",false}}));});
 post("/api/wifi/(scan|connect|hotspot|cancel)",[this](const httplib::Request& req,httplib::Response& r){
  try {
   if(!impl_->options.wifi){jsonResponse(r,{{"error","Wi-Fi unavailable"}},503);return;}
   const std::string action=req.matches[1];
   if(action!="scan"&&status()["alarm"]["critical"].get<bool>()){jsonResponse(r,{{"error","Alarm active; Wi-Fi change unavailable"}},409);return;}
   impl_->options.wifi->request(action,Json::parse(req.body));jsonResponse(r,{{"accepted",true}},202);
  }catch(const std::invalid_argument&){jsonResponse(r,{{"error","Invalid Wi-Fi settings"}},400);}
   catch(const Json::exception&){jsonResponse(r,{{"error","Invalid Wi-Fi settings"}},400);}
   catch(...){jsonResponse(r,{{"error","Wi-Fi operation unavailable or in progress"}},409);}
 });
 get("/.*",[this](const httplib::Request&,httplib::Response& r){
  if(impl_->options.wifi&&impl_->options.wifi->status().value("hotspot",false))r.set_redirect("http://192.168.4.1/",302);
  else r.status=404;
 });
 server.set_exception_handler([](const httplib::Request&,httplib::Response& r,std::exception_ptr){jsonResponse(r,{{"error","Internal request error"}},500);});
 };
 setupServer(p.server);
 if(!p.server.bind_to_port(p.options.bind,p.options.port))throw std::runtime_error("Cannot bind local web interface (port in use?)");
 p.active=true;
 if(p.options.wifi)p.options.wifi->start();
 if(p.options.updates)p.options.updates->start([this]{auto& state=*impl_;std::lock_guard<std::mutex> lock(state.mutex);const auto now=currentClock();const auto w=state.options.wifi?state.options.wifi->status():Json::object();return UpdateContext{w.value("connected",false)&&!w.value("hotspot",false)&&!w.value("busy",false),state.core.view(now).critical,state.configuration.autoUpdateCheck,state.timeBaseUtc+static_cast<std::int64_t>((now.monotonicMs-state.timeBaseMs)/1000),now.monotonicMs-state.startedAt};});
 if(p.options.networkTime)p.timeThread=std::thread([&p]{p.timeLoop();});
 if(p.options.portalPort>0){setupServer(p.portal);if(p.portal.bind_to_port(p.options.bind,p.options.portalPort))p.portalThread=std::thread([&p]{p.portal.listen_after_bind();});}
 p.serverThread=std::thread([&p]{p.server.listen_after_bind();p.active=false;p.wake.notify_all();});
 p.cloudThread=std::thread([&p]{try{p.cloudLoop();}catch(...){std::lock_guard<std::mutex> lock(p.mutex);p.error="Cloud worker initialization failed";p.core.pollFailed(currentClock());}});
}
void Runtime::stop(){auto& p=*impl_;p.active=false;p.wake.notify_all();p.server.stop();p.portal.stop();if(p.options.updates)p.options.updates->stop();if(p.portalThread.joinable())p.portalThread.join();if(p.options.wifi)p.options.wifi->stop();if(p.timeThread.joinable())p.timeThread.join();if(p.cloudThread.joinable())p.cloudThread.join();if(p.serverThread.joinable())p.serverThread.join();}
Frame Runtime::frame(){auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);auto now=p.displayClock(currentClock());p.core.tick(now);auto view=p.core.view(now);if(view.critical)p.previewUntil=0;if(p.options.wifi&&!view.critical){const auto wifi=p.options.wifi->status();if(wifi.value("hotspot",false)||wifi.value("phase",std::string{})=="joining"||wifi.value("show_address",false))return renderWifiSetup(view.config,now,wifi.value("hotspot",false),wifi.value("show_address",false)?wifi.value("ip",std::string{}):std::string{});}return p.previewUntil>now.monotonicMs?previewFrame(p.previewConfig,p.previewMode,now):renderFrame(view,now);}
bool Runtime::consumeSound(int& volume){auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);auto now=currentClock();p.core.tick(now);if(p.soundTestPending&&p.soundUntil>now.monotonicMs&&!p.core.view(now).critical){p.soundTestPending=false;volume=p.soundTestVolume;return true;}volume=p.configuration.core.volume;return p.core.consumeSound(now);}
bool Runtime::soundAllowed(){auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);auto now=currentClock();p.core.tick(now);auto v=p.core.view(now);if(!v.critical&&p.soundUntil>now.monotonicMs)return p.soundTestVolume>0;return v.critical&&(v.alarmMask&~v.acknowledgedMask)&&p.configuration.core.soundEnabled&&p.configuration.core.volume>0;}
void Runtime::acknowledge(){auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);p.soundUntil=0;p.soundTestPending=false;p.core.acknowledge();}
void Runtime::adjustBrightness(int delta){auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);Config cfg=p.configuration;cfg.core.brightness=std::clamp(cfg.core.brightness+delta,0,255);saveConfig(p.options.directory,cfg);p.configuration=cfg;p.core.configure(cfg.core,currentClock());}
Json Runtime::config(){auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);return configJson(p.configuration);}
void Runtime::configure(const Json& patch){
 if(patch.contains("schema")||(patch.contains("time")&&(patch["time"].contains("manual_utc")||patch["time"].contains("manual_saved_utc"))))throw std::invalid_argument("Internal configuration fields are read-only");
 auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);auto next=mergeConfig(p.configuration,patch);auto now=currentClock();
 saveConfig(p.options.directory,next);
 p.previewUntil=0;
 if(!sameAccount(next,p.configuration)){p.core.invalidate(now);p.devices.clear();p.serial.clear();p.lastSuccessMs=0;p.lastSuccessUtc=0;p.error.clear();}
 const bool manualChanged=next.core.manualUtc!=p.configuration.core.manualUtc||next.core.manualSavedUtc!=p.configuration.core.manualSavedUtc;
 p.configuration=next;if(manualChanged)p.anchorManual();p.core.configure(next.core,now);p.core.setSetup(!p.options.demo&&(next.email.empty()||next.password.empty()));++p.generation;p.wake.notify_all();
}
Json Runtime::preview(const Json& options,bool activate){
 auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);auto now=p.displayClock(currentClock());
 const auto mode=options.value("mode",std::string("vitals"));
 const std::vector<std::string> modes={"vitals","sleep0","sleep1","sleep2","sleep3","battery","charging","battery-mid","battery-low","waiting","offline","alarm","info","setup","corners","chase","colors"};
 if(std::find(modes.begin(),modes.end(),mode)==modes.end())throw std::invalid_argument("Invalid preview");
 Json patch=Json::object();for(const char* key:{"palette","preview_brightness","language"})if(options.contains(key))patch[key]=options.at(key);
 auto cfg=mergeConfig(p.configuration,patch);
 if(activate){p.core.tick(now);if(p.core.view(now).critical)throw std::runtime_error("Alarm active; preview unavailable");p.previewConfig=cfg;p.previewMode=mode;p.previewUntil=now.monotonicMs+10000;}
 return displayJson(previewFrame(cfg,mode,now),mode);
}
void Runtime::stopPreview(){auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);p.previewUntil=0;}
void Runtime::testSound(int volume){auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);if(volume<0||volume>6||p.core.view(currentClock()).critical)throw std::invalid_argument("Invalid sound test");p.soundTestVolume=volume;p.soundUntil=currentClock().monotonicMs+1000;p.soundTestPending=true;}
void Runtime::stopSound(){acknowledge();}
void Runtime::resetSettings(){auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);Config next;saveConfig(p.options.directory,next);p.configuration=next;p.anchorManual();p.core=Core(next.core);p.core.setSetup(!p.options.demo);p.devices.clear();p.serial.clear();p.error.clear();p.lastSuccessUtc=0;p.previewUntil=p.soundUntil=0;p.soundTestPending=false;++p.generation;p.wake.notify_all();}
void Runtime::demoScenario(const std::string& scene){
 auto& p=*impl_;if(!p.options.demo)throw std::invalid_argument("Simulation is disabled");
 if(scene!="vitals"&&scene!="charging"&&scene!="offline"&&scene!="alarm"&&scene!="waiting")throw std::invalid_argument("Unknown simulation scenario");
 std::lock_guard<std::mutex> lock(p.mutex);p.scenario=scene;
 if(scene=="offline")p.core.invalidate(currentClock());
 ++p.generation;p.wake.notify_all();
}
Json Runtime::status(){
 auto& p=*impl_;std::lock_guard<std::mutex> lock(p.mutex);auto now=p.displayClock(currentClock());p.core.tick(now);auto view=p.core.view(now);
 if(view.critical)p.previewUntil=0;
 const auto wifi=p.options.wifi?p.options.wifi->status():Json({{"supported",false}});
 const bool networkSetup=!view.critical&&(wifi.value("hotspot",false)||wifi.value("phase",std::string{})=="joining"||wifi.value("show_address",false));
 const bool previewActive=!networkSetup&&p.previewUntil>now.monotonicMs;
 auto frame=networkSetup?renderWifiSetup(view.config,now,wifi.value("hotspot",false),wifi.value("show_address",false)?wifi.value("ip",std::string{}):std::string{}):previewActive?previewFrame(p.previewConfig,p.previewMode,now):renderFrame(view,now);
 Json devices=Json::array();for(const auto& d:p.devices)devices.push_back({{"serial",d.serial},{"name",d.name}});
 Json readings={{"heart_rate",nullptr},{"oxygen",nullptr},{"battery",nullptr},{"sleep_status",nullptr}};
 if(view.vitalsFresh){readings["heart_rate"]=view.vitals.heart;readings["oxygen"]=view.vitals.oxygen;
  switch(sleepState(view.vitals.sleepSt)) {case SleepState::Awake:readings["sleep_status"]="awake";break;case SleepState::Light:readings["sleep_status"]="light_sleep";break;case SleepState::Deep:readings["sleep_status"]="deep_sleep";break;default:readings["sleep_status"]="unknown_sleep";}
 }
 if(view.cloudFresh&&view.vitals.valid)readings["battery"]=view.vitals.battery;
 Json result={{"version",AppVersion},{"target","tc002"},{"mode",p.options.demo?"demo":"live"},{"hardware_verified",false},
 {"connected",view.cloudFresh},{"cloud_fresh",view.cloudFresh},{"vitals_fresh",view.vitalsFresh},{"state_message",view.reason},{"last_error",p.error},{"last_updated",nullptr},
 {"device",{{"serial",p.serial},{"name",p.options.demo?"Simulated Owlet":"Owlet"}}},{"devices",devices},{"readings",readings},
 {"display",displayJson(frame,networkSetup?"wifi":previewActive?"test":screenName(view.screen))},{"preview_active",previewActive},{"preview_mode",previewActive?p.previewMode:""},
 {"sock",{{"charging",view.cloudFresh?Json(bool(view.vitals.charging)):Json(nullptr)},{"removed",view.cloudFresh?Json(view.vitals.sockOff):Json(nullptr)},{"base_on",view.cloudFresh?Json(view.vitals.baseOn):Json(nullptr)},{"hardware",view.cloudFresh?view.vitals.hardware:""}}},
 {"uptime_seconds",(now.monotonicMs-p.startedAt)/1000},{"failure_count",p.failureCount},{"app_active",view.appActive},
 {"alarm",{{"active",!view.alarmText.empty()},{"critical",view.critical},{"message",view.alarmText},{"acknowledged",view.silenced}}},{"poll_count",p.pollCount}};
 result["wifi"]=wifi;
 result["update"]=p.options.updates?p.options.updates->status():Json({{"current",AppVersion},{"install_supported",false},{"phase","unavailable"}});
 const auto displayUtc=now.displayUtcSeconds;
 std::string local;try{local=localDateTime(p.configuration.core.timeZone,displayUtc);}catch(...){}
 result["time"]={{"zone",p.configuration.core.timeZone},{"automatic",p.configuration.core.automaticTime},{"utc",displayUtc},{"local",local},{"synchronized",p.timeSynced},{"source",p.configuration.core.automaticTime?(p.options.demo?"demo":p.timeSynced?"ntp":"device"):"manual"},{"error",p.configuration.core.automaticTime?p.timeError:""}};
 if(p.lastSuccessUtc)result["last_updated"]=isoUtc(p.lastSuccessUtc);return result;
}
}
