// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/owlet.hpp"
#include <cassert>
#include <deque>
#include <iostream>
#include <functional>
#include <limits>
using namespace owlanzi;
namespace {
struct Mock:HttpTransport {
 std::deque<HttpResponse> responses;
 std::vector<HttpRequest> requests;
 mutable bool stopped=false;
 std::size_t stopAfterRequest=0;
 bool cancelled()const override{return stopped;}
 HttpResponse perform(const HttpRequest& r)override {
  requests.push_back(r);if(responses.empty())throw std::runtime_error("Unexpected HTTP request");
  auto v=responses.front();responses.pop_front();
  if(stopAfterRequest&&requests.size()>=stopAfterRequest)stopped=true;
  return v;
 }
 void reply(Json j,int status=200){responses.push_back({status,j.dump()});}
};
bool throws(const std::function<void()>& f){try{f();return false;}catch(...){return true;}}
Json properties(const std::string& stamp="2026-09-09T12:00:00Z") {
 Json v={{"hr",123},{"ox",98},{"bat",75},{"bso",true},{"ss",15},{"chg",0},{"sc",1}};
 return Json::array({{{"property",{{"name","REAL_TIME_VITALS"},{"value",v.dump()},{"data_updated_at",stamp}}}},{{"property",{{"name","LOW_OX_ALRT"},{"value","1"}}}}});
}
void seedLogin(Mock& h,int ttl=3600) {
 h.reply({{"idToken","test-id-token"}});h.reply({{"mini_token","test-mini-token"}});
 h.reply({{"access_token","test-access-token"},{"refresh_token","test-refresh-token"},{"expires_in",ttl}});
 h.reply(Json::array({{{"device",{{"dsn","SOCK-123"},{"product_name","Smart Sock"}}}}}));
}
void seedPoll(Mock& h){h.responses.push_back({201,""});h.reply(properties());}
}
int main(){
 Config original;original.email="example@example.invalid";original.password="fictional-test-password";
 auto sanitized=configJson(original);assert(!sanitized.contains("password"));assert(sanitized.at("has_password")==true);
 assert(mergeConfig(original,{{"password",""},{"brightness",22}}).password==original.password);
 assert(mergeConfig(original,{{"clear_password",true}}).password.empty());
 assert(throws([&]{mergeConfig(original,{{"poll_interval_seconds",1}});}));
 assert(throws([&]{mergeConfig(original,{{"brightness",1.5}});}));
 assert(throws([&]{mergeConfig(original,{{"brightness",4294967326ULL}});})); // Must not narrow to 30.
 assert(throws([&]{mergeConfig(original,{{"brightness",std::numeric_limits<unsigned long long>::max()}});}));
 assert(throws([&]{mergeConfig(original,{{"alarms",{{"spo2_min",4294967382ULL}}}});})); // Must not narrow to 86.
 assert(throws([&]{mergeConfig(original,{{"alarms",{{"enabled",1}}}});}));
 assert(throws([&]{mergeConfig(original,{{"device_serial","../../other"}});}));
 assert(throws([&]{mergeConfig(original,{{"palette",{{"heart","#hello!"}}}});}));
 auto v=parseProperties(properties());assert(v.lowOx&&v.valid&&v.heart==123&&v.measuredAt==parseUtc("2026-09-09T12:00:00Z"));
 auto invalid=properties();invalid[0]["property"]["value"]="{\"hr\":999,\"ox\":98}";assert(throws([&]{parseProperties(invalid);}));
 assert(throws([&]{parseProperties(Json::object());}));
 assert(throws([&]{parseProperties(Json::array());}));
 auto duplicate=properties();duplicate.push_back(duplicate[0]);assert(throws([&]{parseProperties(duplicate);}));
 for(const Json& sleep : {Json(1.5),Json(4294967297ULL)}) {
  auto malformed=properties();auto raw=Json::parse(malformed[0]["property"]["value"].get<std::string>());
  raw["ss"]=sleep;malformed[0]["property"]["value"]=raw.dump();
  assert(throws([&]{parseProperties(malformed);})); // Never turn malformed/overflowed sleep into awake.
 }
 auto unknownSleep=properties();auto unknownRaw=Json::parse(unknownSleep[0]["property"]["value"].get<std::string>());
 unknownRaw["ss"]=42;unknownSleep[0]["property"]["value"]=unknownRaw.dump();
 assert(sleepState(parseProperties(unknownSleep).sleepSt)==SleepState::Unknown);
 auto malformedFlag=properties();malformedFlag[1]["property"]["value"]=Json::object({{"unexpected",true}});
 assert(throws([&]{parseProperties(malformedFlag);})); // A malformed flag must not silently become cleared.
 auto clock=Clock{1000,parseUtc("2026-09-09T12:00:00Z")};
 Mock h;seedLogin(h);seedPoll(h);OwletClient client(h);client.configure(original);auto result=client.poll(clock);
 assert(result.appActive&&result.vitals.heart==123&&client.serial()=="SOCK-123");assert(h.requests.size()==6);
 assert(h.requests[1].headers.at(0).second=="test-id-token"); // SSO uses bare JWT.
 assert(h.requests[4].url.find("APP_ACTIVE/datapoints.json")!=std::string::npos);
 assert(h.requests[5].headers[0].second=="auth_token test-access-token");
 seedPoll(h);client.poll({6000,clock.utcSeconds+5});assert(h.requests.size()==8); // session reused.
 Mock renew;seedLogin(renew,1);seedPoll(renew);OwletClient refresh(renew);refresh.configure(original);refresh.poll(clock);
 renew.reply({{"access_token","new-access"},{"expires_in",3600}});seedPoll(renew);refresh.poll({3000,clock.utcSeconds+2});
 assert(renew.requests[6].url.find("refresh_token.json")!=std::string::npos);assert(renew.requests[7].headers[0].second=="auth_token new-access");
 Mock bad;seedLogin(bad);bad.responses.push_back({500,"private-body-must-not-leak"});bad.reply(properties());
 OwletClient activation(bad);activation.configure(original);auto activationResult=activation.poll(clock);
 assert(!activationResult.appActive&&activationResult.vitals.lowOx&&activationResult.vitals.valid);
 assert(bad.requests.size()==6); // A timestamp-valid critical flag remains available despite an APP_ACTIVE-only failure.
 for(unsigned attempt=1;attempt<5;++attempt) {
  bad.responses.push_back({500,"private-activation-response"});bad.reply(properties());
  auto partial=activation.poll({1000+attempt*5000,clock.utcSeconds+attempt*5});
  assert(!partial.appActive&&partial.vitals.lowOx);
 }
 assert(activation.serial().empty());const auto afterFailures=bad.requests.size();
 seedLogin(bad);seedPoll(bad);assert(activation.poll({31000,clock.utcSeconds+30}).appActive);
 assert(bad.requests[afterFailures].url.find("verifyPassword")!=std::string::npos);
 Mock multi;multi.reply({{"idToken","id"}});multi.reply({{"mini_token","mini"}});multi.reply({{"access_token","token"}});
 multi.reply(Json::array({{{"device",{{"dsn","A"}}}},{{"device",{{"dsn","B"}}}}}));OwletClient choose(multi);choose.configure(original);
 assert(throws([&]{choose.poll(clock);}));assert(choose.devices().size()==2);assert(choose.serial().empty());
 auto changed=original;changed.europe=false;Mock world;seedLogin(world);seedPoll(world);OwletClient w(world);w.configure(changed);w.poll(clock);
 assert(world.requests[1].url=="https://ayla-sso.owletdata.com/mini/");
 // Presentation-only configuration changes keep the cloud account session.
 auto cosmetic=original;cosmetic.core.brightness=40;client.configure(cosmetic);seedPoll(h);client.poll({11000,clock.utcSeconds+10});
 assert(h.requests.size()==10);
 // An account switch discards both tokens and selected device immediately.
 auto account=cosmetic;account.email="different@example.invalid";client.configure(account);
 assert(client.serial().empty()&&client.devices().empty());seedLogin(h);seedPoll(h);client.poll({16000,clock.utcSeconds+15});
 assert(h.requests.size()==16&&h.requests[10].url.find("verifyPassword")!=std::string::npos);
 assert(Json::parse(h.requests[10].body).at("email")==account.email);
 // A failed refresh performs a complete login, and reselects a device after 401.
 Mock fallback;seedLogin(fallback,1);seedPoll(fallback);OwletClient recover(fallback);recover.configure(original);recover.poll(clock);
 fallback.responses.push_back({401,"private-refresh-response"});seedLogin(fallback);seedPoll(fallback);
 recover.poll({3000,clock.utcSeconds+2});assert(fallback.requests.size()==13);
 assert(fallback.requests[6].url.find("refresh_token.json")!=std::string::npos);
 assert(fallback.requests[7].url.find("verifyPassword")!=std::string::npos);
 // Response bodies and credentials must never be embedded in surfaced errors.
 Mock privateError;privateError.responses.push_back({403,"fictional-test-password private-cloud-body"});
 OwletClient protectedClient(privateError);protectedClient.configure(original);
 try {protectedClient.poll(clock);assert(false);}catch(const std::exception& e) {
  const std::string error=e.what();assert(error.find(original.password)==std::string::npos);
  assert(error.find("private-cloud-body")==std::string::npos&&error.find("HTTP 403")!=std::string::npos);
 }
 // Malformed token types fail closed; no property request can use a bogus token.
 Mock malformedToken;malformedToken.reply({{"idToken","test-id-token"}});malformedToken.reply({{"mini_token","test-mini-token"}});
 malformedToken.reply({{"access_token",Json::array({"private-token"})}});OwletClient malformedClient(malformedToken);malformedClient.configure(original);
 assert(throws([&]{malformedClient.poll(clock);}));assert(malformedToken.requests.size()==3);
 // Shutdown between any login/setup step prevents the next transport call.
 for(std::size_t phase=0;phase<=5;++phase) {
  Mock stopped;seedLogin(stopped);seedPoll(stopped);stopped.stopped=phase==0;stopped.stopAfterRequest=phase;
  OwletClient stopping(stopped);stopping.configure(original);
  assert(throws([&]{stopping.poll(clock);}));assert(stopped.requests.size()==phase);
 }
 // Refresh failure must not enter the fallback login chain after cancellation.
 Mock stopRefresh;seedLogin(stopRefresh,1);seedPoll(stopRefresh);OwletClient stoppedRefresh(stopRefresh);
 stoppedRefresh.configure(original);stoppedRefresh.poll(clock);
 stopRefresh.responses.push_back({401,"cancelled-refresh-body"});stopRefresh.stopAfterRequest=7;
 assert(throws([&]{stoppedRefresh.poll({3000,clock.utcSeconds+2});}));
 assert(stopRefresh.requests.size()==7&&stopRefresh.requests.back().url.find("refresh_token.json")!=std::string::npos);
 std::cout<<"services tests passed (strict configuration, parsing, authentication, refresh recovery, account invalidation, activation diagnostics)\n";
}
