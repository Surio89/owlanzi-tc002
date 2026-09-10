// SPDX-License-Identifier: GPL-3.0-or-later
// Owlet protocol facts from the existing Owlanzi client, pyowletapi and
// owlet_monitor. These public app identifiers are not user credentials.
#include "owlanzi/owlet.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <limits>
namespace owlanzi {
namespace {
struct Region {const char *key,*app,*secret,*mini,*signin,*refresh,*base;};
const Region EU={"AIzaSyDm6EhV70wudwN3iOSq3vTjtsdGjdFLuuM","OwletCare-Android-EU-fw-id","OwletCare-Android-EU-JKupMPBoj_Npce_9a95Pc8Qo0Mw","https://ayla-sso.eu.owletdata.com/mini/","https://user-field-eu-1a2039d9.aylanetworks.com/api/v1/token_sign_in","https://user-field-eu-1a2039d9.aylanetworks.com/users/refresh_token.json","https://ads-field-eu-1a2039d9.aylanetworks.com/apiv1"};
const Region World={"AIzaSyCsDZ8kWxQuLJAMVnmEhEkayH1TSxKXfGA","sso-prod-3g-id","sso-prod-UEjtnPCtFfjdwIwxqnC0OipxRFU","https://ayla-sso.owletdata.com/mini/","https://user-field-1a2039d9.aylanetworks.com/api/v1/token_sign_in","https://user-field-1a2039d9.aylanetworks.com/users/refresh_token.json","https://ads-field-1a2039d9.aylanetworks.com/apiv1"};
bool flag(const Json& v) {
 if(v.is_null())return false;
 if(v.is_boolean())return v.get<bool>();
 if(v.is_number_integer())return v!=0;
 if(v.is_string()&&(v=="true"||v=="1"))return true;
 if(v.is_string()&&(v=="false"||v=="0"))return false;
 throw std::runtime_error("Invalid boolean measurement field");
}
int integerField(const Json& j,const char* key,int fallback=0) {
 if(!j.contains(key))return fallback;const auto& v=j.at(key);
 if(!v.is_number_integer()||(v.is_number_unsigned()?v.get<std::uint64_t>()>static_cast<std::uint64_t>(std::numeric_limits<int>::max()):(v.get<std::int64_t>()<std::numeric_limits<int>::min()||v.get<std::int64_t>()>std::numeric_limits<int>::max())))throw std::runtime_error("Invalid integer cloud field");
 return v.get<int>();
}
bool serialValid(const std::string& s){return !s.empty()&&s.size()<=64&&s.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-")==std::string::npos;}
std::string textValue(const Json& j,const char* key,std::size_t max=16384) {
 if(!j.contains(key)||!j.at(key).is_string())throw std::runtime_error("Cloud response is missing a required field");
 auto value=j.at(key).get<std::string>();
 if(value.empty()||value.size()>max||value.find_first_of("\r\n")!=std::string::npos)throw std::runtime_error("Cloud response contains an invalid field");return value;
}
}
Vitals parseProperties(const Json& properties) {
 if(!properties.is_array())throw std::runtime_error("Properties response must be an array");
 Vitals v;Json raw;bool found=false;
 for(const auto& item:properties) {
  if(!item.is_object()||!item.contains("property")||!item.at("property").is_object())continue;
  const auto& p=item.at("property");if(!p.contains("name")||!p.at("name").is_string()||!p.contains("value"))continue;
  auto name=p.at("name").get<std::string>();const auto& val=p.at("value");
  if(name=="REAL_TIME_VITALS") {
   if(found)throw std::runtime_error("Duplicate measurement property");found=true;
   raw=val.is_string()?Json::parse(val.get<std::string>(),nullptr,false):val;
   v.measuredAt=parseUtc(p.value("data_updated_at",std::string{}));
  }
  else if(name=="LOW_OX_ALRT")v.lowOx=flag(val);else if(name=="HIGH_OX_ALRT")v.highOx=flag(val);
  else if(name=="LOW_HR_ALRT")v.lowHr=flag(val);else if(name=="HIGH_HR_ALRT")v.highHr=flag(val);
  else if(name=="CRIT_OX_ALRT")v.criticalOx=flag(val);else if(name=="CRIT_BATT_ALRT")v.criticalBatt=flag(val);
  else if(name=="LOST_POWER_ALRT")v.lostPower=flag(val);else if(name=="SOCK_DISCON_ALRT")v.sockDiscon=flag(val);
  else if(name=="SOCK_OFF")v.sockOff=flag(val);else if(name=="LOW_BATT_ALRT")v.lowBatt=flag(val);
 }
 if(!found||!raw.is_object())throw std::runtime_error("Missing or invalid REAL_TIME_VITALS");
 try {
  v.heart=raw.value("hr",0.0f);v.oxygen=raw.value("ox",0.0f);v.battery=raw.value("bat",0.0f);v.oxygen10=raw.value("oxta",0.0f);
  // Owlet uses 255 when its optional averaged oxygen field is unavailable
  // (e.g. on the charger). Keep the existing missing-value representation;
  // this field is never used for displayed readings or alert thresholds.
  if(v.oxygen10==255.f) v.oxygen10=0;
  v.baseOn=flag(raw.value("bso",Json{}));v.sleepSt=integerField(raw,"ss");v.charging=flag(raw.value("chg",Json{}));v.sockConn=flag(raw.value("sc",Json{}));v.movement=flag(raw.value("mv",Json{}));
  v.hardware=raw.value("hw",std::string{});v.valid=true;
 }catch(const Json::exception&) {throw std::runtime_error("Invalid measurement fields");}
 if(!validVitals(v)) {
  // Report only the field and category, never raw readings or cloud payloads.
  for(const auto& field : {std::make_pair("hr",std::make_pair(v.heart,400.f)),
                          std::make_pair("ox",std::make_pair(v.oxygen,100.f)),
                          std::make_pair("bat",std::make_pair(v.battery,100.f)),
                          std::make_pair("oxta",std::make_pair(v.oxygen10,100.f))}) {
   const float value=field.second.first;
   if(!std::isfinite(value)||value<0||value>field.second.second)
    throw std::runtime_error(std::string("Invalid cloud field ")+field.first+
     (!std::isfinite(value)?": non-finite":value<0?": negative":": above range")+
     (v.charging?" (charging)":""));
  }
  throw std::runtime_error("Measurements outside supported range");
 }
 return v;
}
void OwletClient::configure(const Config& c) {
 if(!sameAccount(c,config_)) {access_.clear();refresh_.clear();serial_.clear();devices_.clear();expires_=0;activationFailures_=0;}config_=c;
}
Json OwletClient::request(const std::string& stage,const std::string& method,const std::string& url,const Json* body,const std::vector<std::pair<std::string,std::string>>& headers,bool parse) {
 if(transport_.cancelled())throw std::runtime_error("Request cancelled");
 auto r=transport_.perform({method,url,body?body->dump():"",headers});
 if(r.status<200||r.status>=300) {
  if(r.status==401){access_.clear();serial_.clear();expires_=0;}
  // Never include response bodies, URLs, account details or tokens in errors.
  throw std::runtime_error(stage+": HTTP "+std::to_string(r.status));
 }
 if(!parse)return Json::object();
 auto j=Json::parse(r.body,nullptr,false);if(j.is_discarded())throw std::runtime_error(stage+": invalid JSON");return j;
}
void OwletClient::token(const Json& j,Clock now) {
 auto a=textValue(j,"access_token");std::string r=refresh_;
 if(j.contains("refresh_token")&&!j.at("refresh_token").is_null())r=textValue(j,"refresh_token");
 auto ttl=integerField(j,"expires_in",3600);if(ttl<=0)throw std::runtime_error("Invalid cloud token lifetime");ttl=std::min(86400,ttl);if(ttl>120)ttl-=120;
 access_=a;refresh_=r;expires_=now.monotonicMs+static_cast<std::uint64_t>(ttl)*1000;
}
void OwletClient::login(Clock now) {
 const auto& region=config_.europe?EU:World;
 Json credentials={{"email",config_.email},{"password",config_.password},{"returnSecureToken",true}};
 auto id=textValue(request("Firebase","POST",std::string("https://www.googleapis.com/identitytoolkit/v3/relyingparty/verifyPassword?key=")+region.key,&credentials,{{"X-Android-Package","com.owletcare.owletcare"},{"X-Android-Cert","2A3BC26DB0B8B0792DBE28E6FFDC2598F9B12B74"}}),"idToken");
 auto mini=textValue(request("Owlet SSO","GET",region.mini,nullptr,{{"Authorization",id}}),"mini_token");
 Json sign={{"app_id",region.app},{"app_secret",region.secret},{"provider","owl_id"},{"token",mini}};
 token(request("Ayla","POST",region.signin,&sign),now);
}
bool OwletClient::refresh(Clock now) {
 if(refresh_.empty())return false;Json body={{"user",{{"refresh_token",refresh_}}}};
 try {token(request("Refresh","POST",config_.europe?EU.refresh:World.refresh,&body),now);return true;}catch(...){access_.clear();return false;}
}
void OwletClient::findDevice() {
 auto list=request("Device list","GET",std::string(config_.europe?EU.base:World.base)+"/devices.json",nullptr,{{"Authorization","auth_token "+access_}});
 if(!list.is_array())throw std::runtime_error("Invalid device list");devices_.clear();serial_.clear();
 for(const auto& item:list) {
  if(!item.is_object()||!item.contains("device")||!item.at("device").is_object())continue;const auto& d=item.at("device");
  auto s=d.value("dsn",std::string{});if(!serialValid(s))continue;
  devices_.push_back({s,d.value("product_name",std::string("Owlet"))});
 }
 if(devices_.empty())throw std::runtime_error("No paired Owlet devices found");
 if(config_.deviceSerial.empty()&&devices_.size()==1)serial_=devices_[0].serial;
 else for(const auto& d:devices_)if(d.serial==config_.deviceSerial)serial_=d.serial;
 if(serial_.empty())throw std::runtime_error("Select a paired Owlet device in settings");
}
PollResult OwletClient::poll(Clock now) {
 try {
 if(config_.email.empty()||config_.password.empty())throw std::runtime_error("Enter your Owlet account in settings");
 if(now.utcSeconds<1700000000)throw std::runtime_error("Waiting for a valid system clock");
 if(access_.empty()||now.monotonicMs>=expires_) {if(!refresh(now))login(now);}
 if(serial_.empty())findDevice();
 auto base=std::string(config_.europe?EU.base:World.base)+"/dsns/"+serial_;
 std::vector<std::pair<std::string,std::string>> auth={{"Authorization","auth_token "+access_}};
 Json active={{"datapoint",{{"metadata",Json::object()},{"value",1}}}};
 // Continue reading properties if activation alone fails: new critical flags
 // must still reach the display. Measurement timestamps always govern freshness.
 bool activated=true;
 try {request("APP_ACTIVE","POST",base+"/properties/APP_ACTIVE/datapoints.json",&active,auth,false);}
 catch(const std::exception&){activated=false;}
 auto properties=request("Properties","GET",base+"/properties.json",nullptr,auth);
 auto vitals=parseProperties(properties);
 if(activated)activationFailures_=0;
 else if(++activationFailures_>=5){access_.clear();refresh_.clear();serial_.clear();expires_=0;activationFailures_=0;}
 return {vitals,activated};
 }catch(const Json::exception&){throw std::runtime_error("Invalid cloud response fields");}
}
}
