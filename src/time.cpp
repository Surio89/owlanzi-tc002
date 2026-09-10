// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/time.hpp"
#include "owlanzi/core.hpp"
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <stdexcept>
namespace owlanzi {
namespace {
#include "timezones.inc"
const Zone* find(const std::string& name){for(const auto& z:Zones)if(name==z.name)return &z;return nullptr;}
}
bool validTimeZone(const std::string& name){return find(name)!=nullptr;}
std::vector<std::string> timeZones(){std::vector<std::string> result;for(const auto& z:Zones)result.push_back(z.name);return result;}
int timeZoneOffset(const std::string& name,std::int64_t utc){
 const auto* z=find(name);if(!z||utc<1577836800LL||utc>=4102444800LL)throw std::invalid_argument("Unsupported time or timezone");
 auto next=std::upper_bound(z->entries,z->entries+z->count,utc,[](auto t,const auto& item){return t<item.at;});return (next-1)->offset;
}
std::string localDateTime(const std::string& zone,std::int64_t utc){
 auto seconds=utc+timeZoneOffset(zone,utc);
 // Convert with the existing platform-independent UTC calendar routines. On
 // 32-bit ARM time_t cannot represent dates after 2038; use a 28-year calendar
 // cycle inside 2001..2036 (the supported range contains no non-leap century).
 int years=0;constexpr std::int64_t cycle=883612800LL;
 while(seconds>=2114380800LL){seconds-=cycle;years+=28;}
 std::time_t value=static_cast<std::time_t>(seconds);std::tm tm{};
#ifdef _WIN32
 gmtime_s(&tm,&value);
#else
 gmtime_r(&value,&tm);
#endif
 char out[32];std::snprintf(out,sizeof(out),"%04d-%02d-%02dT%02d:%02d:%02d",tm.tm_year+1900+years,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);return out;
}
std::string clockText(const std::string& zone,std::int64_t utc){try{return localDateTime(zone,utc).substr(11,5);}catch(...){return "--:--";}}
std::int64_t localToUtc(const std::string& name,const std::string& local){
 const auto* z=find(name);if(!z||local.size()!=16||local[10]!='T')throw std::invalid_argument("Invalid local date/time");
 const auto wall=parseUtc(local+":00Z");if(!wall)throw std::invalid_argument("Invalid local date/time");
 std::int64_t result=0;
 for(std::size_t i=0;i<z->count;++i){const auto utc=wall-z->entries[i].offset;try{if(localDateTime(name,utc).substr(0,16)==local&&(!result||utc<result))result=utc;}catch(const std::invalid_argument&) {}}
 if(!result)throw std::invalid_argument("Invalid date or nonexistent daylight-saving time");return result;
}
}
