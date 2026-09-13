// SPDX-License-Identifier: GPL-3.0-or-later
#include <array>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <limits>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
namespace owlanzi {
std::int64_t tc002NetworkTime(){
 addrinfo hints{},*info=nullptr;hints.ai_family=AF_INET;hints.ai_socktype=SOCK_DGRAM;
 if(getaddrinfo("pool.ntp.org","123",&hints,&info)!=0||!info)throw std::runtime_error("Time server unavailable");
 const int fd=socket(info->ai_family,SOCK_DGRAM,0);if(fd<0){freeaddrinfo(info);throw std::runtime_error("Time socket unavailable");}
 timeval timeout{3,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
 const bool connected=connect(fd,info->ai_addr,info->ai_addrlen)==0;freeaddrinfo(info);
 std::array<unsigned char,48> request{},reply{};request[0]=0x23;
 const auto stamp=static_cast<std::uint32_t>(std::time(nullptr)+2208988800LL);
 for(int i=0;i<4;++i)request[40+i]=static_cast<unsigned char>(stamp>>(24-8*i));
 const int random=open("/dev/urandom",O_RDONLY);const bool nonce=random>=0&&read(random,request.data()+44,4)==4;if(random>=0)close(random);
 const auto count=connected&&nonce&&send(fd,request.data(),request.size(),0)==48?recv(fd,reply.data(),reply.size(),0):-1;close(fd);
 if(count!=48||(reply[0]&7)!=4||((reply[0]>>3)&7)<3||((reply[0]>>3)&7)>4||(reply[0]>>6)==3||reply[1]==0||reply[1]>15||std::memcmp(reply.data()+24,request.data()+40,8)!=0)throw std::runtime_error("Invalid time response");
 std::int64_t seconds=0;for(int i=40;i<44;++i)seconds=(seconds<<8)|reply[i];
 if(seconds==0)throw std::runtime_error("Empty time response");
 // Resolve the 2036 NTP era rollover within the supported 2020..2100 range.
 seconds-=2208988800LL;if(seconds<1577836800LL)seconds+=4294967296LL;
 if(seconds<1577836800LL||seconds>=4102444800LL||seconds>std::numeric_limits<std::time_t>::max())throw std::runtime_error("Unsupported network date");
 // A cold TC002 starts its system clock at 1970. Updating only the displayed
 // time leaves Owlet's freshness checks and TLS certificate validation blocked.
 // This adapter runs on the clock only; the desktop runtime never sets OS time.
 const timespec utc{static_cast<std::time_t>(seconds),0};
 if(clock_settime(CLOCK_REALTIME,&utc)!=0)throw std::runtime_error("System clock synchronization failed");
 return seconds;
}
}
