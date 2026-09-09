// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "owlanzi/config.hpp"
#include "owlanzi/http.hpp"
namespace owlanzi {
struct Device {std::string serial,name;};
struct PollResult {Vitals vitals;bool appActive=false;};
Vitals parseProperties(const Json& properties);
class OwletClient {
public:
 explicit OwletClient(HttpTransport& transport):transport_(transport){}
 void configure(const Config& config);
 PollResult poll(Clock now);
 const std::vector<Device>& devices()const{return devices_;}
 std::string serial()const{return serial_;}
private:
 Json request(const std::string& stage,const std::string& method,const std::string& url,const Json* body=nullptr,const std::vector<std::pair<std::string,std::string>>& headers={},bool parse=true);
 void login(Clock now);bool refresh(Clock now);void token(const Json& response,Clock now);void findDevice();
 Config config_;HttpTransport& transport_;std::string access_,refresh_,serial_;std::uint64_t expires_=0;
 std::vector<Device> devices_;unsigned activationFailures_=0;
};
}
