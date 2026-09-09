// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/config.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <array>
#include <limits>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#else
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif
namespace owlanzi {
namespace {
using Member = std::uint32_t Palette::*;
const std::pair<const char*, Member> colors[] = {
 {"heart",&Palette::heart},{"numbers",&Palette::numbers},{"separator",&Palette::separator},
 {"waiting",&Palette::waiting},{"awake",&Palette::awake},{"light_sleep",&Palette::light_sleep},
 {"deep_sleep",&Palette::deep_sleep},{"unknown_sleep",&Palette::unknown_sleep},
 {"battery",&Palette::battery},{"alarm",&Palette::alarm},{"info",&Palette::info},{"offline",&Palette::offline}};
void secureDirectory(const std::string& directory) {
 std::filesystem::create_directories(directory);
#ifndef _WIN32
 if(chmod(directory.c_str(),0700)!=0) throw std::runtime_error("Cannot secure data directory");
#endif
}
std::string randomHex() {
 std::array<unsigned char,32> bytes{};
#ifdef _WIN32
 if(BCryptGenRandom(nullptr,bytes.data(),static_cast<ULONG>(bytes.size()),BCRYPT_USE_SYSTEM_PREFERRED_RNG)!=0)
   throw std::runtime_error("Cannot generate pairing token");
#else
 std::ifstream random("/dev/urandom",std::ios::binary);
 if(!random.read(reinterpret_cast<char*>(bytes.data()),bytes.size())) throw std::runtime_error("Cannot generate pairing token");
#endif
 std::ostringstream out; out << std::hex << std::setfill('0');
 for(auto b:bytes) out << std::setw(2) << static_cast<unsigned>(b);
 return out.str();
}
}
std::string colorHex(std::uint32_t rgb) {
 std::ostringstream out; out << '#' << std::hex << std::uppercase << std::setw(6) << std::setfill('0') << (rgb&0xffffff); return out.str();
}
Json configJson(const Config& cfg, bool privateValues) {
 const auto& c=cfg.core;
 Json palette=Json::object(); for(auto entry:colors) palette[entry.first]=colorHex(c.palette.*entry.second);
 Json j={{"region",cfg.europe?"eu":"world"},{"email",cfg.email},{"has_password",!cfg.password.empty()},
 {"device_serial",cfg.deviceSerial},{"language",c.german?"de":"en"},{"poll_interval_seconds",c.pollSeconds},
 {"brightness",c.brightness},{"palette",palette},{"alarms",{
 {"enabled",c.ownAlarms},{"spo2_min",c.spo2Limit},{"heart_rate_min",c.hrLowLimit},{"heart_rate_max",c.hrHighLimit},
 {"spo2_seconds",c.spo2Seconds},{"heart_rate_low_seconds",c.hrLowSeconds},{"heart_rate_high_seconds",c.hrHighSeconds},
 {"sound_enabled",c.soundEnabled},{"volume",c.volume},{"alarm_repeat_seconds",c.alarmRepeatSeconds},{"alarm_brightness",c.alarmBrightness}}}};
 if(privateValues) {j["password"]=cfg.password; j["schema"]=1;}
 return j;
}
Config mergeConfig(const Config& current, const Json& p) {
 if(!p.is_object()) throw std::invalid_argument("Configuration must be an object");
 Config cfg=current; auto& c=cfg.core;
 try {
 if(p.contains("schema") && p.at("schema")!=1) throw std::invalid_argument("Unsupported configuration schema");
 if(p.contains("email")) cfg.email=p.at("email").get<std::string>();
 if(p.contains("password")) {auto v=p.at("password").get<std::string>(); if(!v.empty()) cfg.password=v;}
 if(p.value("clear_password",false)) cfg.password.clear();
 if(p.contains("device_serial")) cfg.deviceSerial=p.at("device_serial").get<std::string>();
 if(p.contains("region")) {auto v=p.at("region").get<std::string>(); if(v!="eu"&&v!="world") throw std::invalid_argument("Invalid region"); cfg.europe=v=="eu";}
 if(p.contains("language")) {auto v=p.at("language").get<std::string>(); if(v!="de"&&v!="en") throw std::invalid_argument("Invalid language"); c.german=v=="de";}
 auto integer=[](const Json& j,const char* key,int& out) {if(j.contains(key)) {
  const auto& v=j.at(key);
  if(!v.is_number_integer() || (v.is_number_unsigned()?v.get<std::uint64_t>()>static_cast<std::uint64_t>(std::numeric_limits<int>::max()):(v.get<std::int64_t>()<std::numeric_limits<int>::min()||v.get<std::int64_t>()>std::numeric_limits<int>::max())))
   throw std::invalid_argument("Expected integer setting in supported range");
  out=v.get<int>();
 }};
 integer(p,"poll_interval_seconds",c.pollSeconds); integer(p,"brightness",c.brightness);
 if(p.contains("alarms")) {
  const auto& a=p.at("alarms"); if(!a.is_object()) throw std::invalid_argument("Invalid alarms");
  if(a.contains("enabled")) c.ownAlarms=a.at("enabled").get<bool>();
  if(a.contains("sound_enabled")) c.soundEnabled=a.at("sound_enabled").get<bool>();
  integer(a,"spo2_min",c.spo2Limit); integer(a,"spo2_seconds",c.spo2Seconds);
  integer(a,"heart_rate_min",c.hrLowLimit); integer(a,"heart_rate_max",c.hrHighLimit);
  integer(a,"heart_rate_low_seconds",c.hrLowSeconds); integer(a,"heart_rate_high_seconds",c.hrHighSeconds);
  integer(a,"volume",c.volume); integer(a,"alarm_repeat_seconds",c.alarmRepeatSeconds); integer(a,"alarm_brightness",c.alarmBrightness);
 }
 if(p.contains("palette")) {
  const auto& a=p.at("palette"); if(!a.is_object()) throw std::invalid_argument("Invalid palette");
  for(auto entry:colors) if(a.contains(entry.first)) {
   auto hex=a.at(entry.first).get<std::string>();
   if(hex.size()!=7||hex[0]!='#'||hex.find_first_not_of("0123456789abcdefABCDEF",1)!=std::string::npos) throw std::invalid_argument("Invalid color");
   c.palette.*entry.second=static_cast<std::uint32_t>(std::stoul(hex.substr(1),nullptr,16));
  }
 }
 } catch(const Json::exception&) {throw std::invalid_argument("Invalid configuration value");}
 if(cfg.email.size()>254 || cfg.password.size()>512 || cfg.deviceSerial.size()>64 ||
   cfg.email.find_first_of("\r\n")!=std::string::npos || cfg.deviceSerial.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")!=std::string::npos || !validCoreConfig(c))
    throw std::invalid_argument("Configuration is outside the supported range");
 return cfg;
}
bool sameAccount(const Config&a,const Config&b) {return a.email==b.email&&a.password==b.password&&a.deviceSerial==b.deviceSerial&&a.europe==b.europe;}
std::string readPrivateFile(const std::string& path,std::size_t limit) {
 std::ifstream in(path,std::ios::binary); if(!in) throw std::runtime_error("Cannot read local configuration");
 std::string value; char block[4096];
 while(in) {in.read(block,sizeof(block)); value.append(block,static_cast<std::size_t>(in.gcount())); if(value.size()>limit) throw std::runtime_error("Local file is too large");}
 return value;
}
void savePrivateFile(const std::string& path,const std::string& content) {
 auto temp=path+".tmp-"+randomHex().substr(0,16);
#ifdef _WIN32
 HANDLE f=CreateFileA(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(f==INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot save local configuration");
 DWORD written=0; bool ok=WriteFile(f,content.data(),static_cast<DWORD>(content.size()),&written,nullptr)&&written==content.size()&&FlushFileBuffers(f);
 CloseHandle(f);
 if(!ok||!MoveFileExA(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {DeleteFileA(temp.c_str());throw std::runtime_error("Cannot commit local configuration");}
#else
 int f=open(temp.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600); if(f<0) throw std::runtime_error("Cannot save local configuration");
 std::size_t done=0; bool ok=true;
 while(done<content.size()) {auto n=write(f,content.data()+done,content.size()-done); if(n<=0){ok=false;break;} done+=static_cast<std::size_t>(n);}
 ok=fsync(f)==0&&ok; close(f);
 if(!ok||rename(temp.c_str(),path.c_str())!=0) {unlink(temp.c_str());throw std::runtime_error("Cannot commit local configuration");}
 auto parent=std::filesystem::path(path).parent_path().string(); int d=open(parent.c_str(),O_RDONLY|O_DIRECTORY); if(d>=0){fsync(d);close(d);}
#endif
}
Config loadConfig(const std::string& dir) {
 secureDirectory(dir); auto file=dir+"/config.json";
 if(!std::filesystem::exists(file)) return {};
 try {return mergeConfig({},Json::parse(readPrivateFile(file)));} catch(...) {throw std::runtime_error("Invalid saved configuration; preserve the file and repair it before starting");}
}
void saveConfig(const std::string& dir,const Config& cfg) {secureDirectory(dir);savePrivateFile(dir+"/config.json",configJson(cfg,true).dump(2));}
std::string loadOrCreateToken(const std::string& dir) {
 secureDirectory(dir); auto path=dir+"/pairing-token";
 if(std::filesystem::exists(path)) {auto token=readPrivateFile(path,128); if(token.size()!=64||token.find_first_not_of("0123456789abcdef")!=std::string::npos) throw std::runtime_error("Invalid pairing token file"); return token;}
 auto token=randomHex();savePrivateFile(path,token);return token;
}
}
