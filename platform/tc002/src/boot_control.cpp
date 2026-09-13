// SPDX-License-Identifier: GPL-3.0-or-later
// Permanent /res entry point. No partition writes and no automatic stock fallback.
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <json.hpp>

namespace {
constexpr const char* Root="/data/owlanzi-app/";
constexpr const char* Stock="/res/etc/owlanzi-stock.cfg";
constexpr const char* Marker="/data/owlanzi-app/manufacturer-selected";
bool regular(const char* path){struct stat s{};return lstat(path,&s)==0&&S_ISREG(s.st_mode);}
void prop(const char* key,const char* value){pid_t p=fork();if(p==0){execl("/bin/setprop","setprop",key,value,static_cast<char*>(nullptr));_exit(127);}if(p>0)waitpid(p,nullptr,0);}
std::string read(const char* path){int fd=open(path,O_RDONLY|O_NOFOLLOW);if(fd<0)return {};std::string out;char b[1024];ssize_t n;while((n=::read(fd,b,sizeof(b)))>0&&out.size()<16384)out.append(b,n);close(fd);return n==0?out:std::string{};}
bool valid(const std::string& text,bool stock){
 try{const auto j=nlohmann::json::parse(text);const auto lib=j.at("startupLibPath").get<std::string>(),ui=j.at("resPath").get<std::string>();
  if(stock)return lib=="/res/lib/libzkgui.so"&&ui=="/res/ui/"&&regular(lib.c_str());
  for(const char* s:{"a","b"})if(lib==std::string(Root)+s+"/lib/libzkgui.so"&&ui==std::string(Root)+s+"/ui/"&&regular(lib.c_str()))return true;
 }catch(...){}return false;
}
bool write(const char* path,const std::string& text){const auto temp=std::string(path)+".next";int fd=open(temp.c_str(),O_WRONLY|O_CREAT|O_TRUNC|O_NOFOLLOW,0600);if(fd<0)return false;std::size_t done=0;while(done<text.size()){auto n=::write(fd,text.data()+done,text.size()-done);if(n<=0){close(fd);return false;}done+=n;}bool ok=fsync(fd)==0;close(fd);if(ok)ok=rename(temp.c_str(),path)==0;int dir=open(Root,O_RDONLY|O_DIRECTORY);if(dir>=0){fsync(dir);close(dir);}return ok;}
// The retained manufacturer's HTTP server gives up if port 80 cannot bind.
// A stopped Owlanzi process may still leave TIME_WAIT connections on that port.
// Probe without SO_REUSEADDR, matching the stricter incoming server, and keep
// the vendor startup watchdog satisfied while those connections expire.
bool waitStockWebPort(){
 for(int attempt=0;attempt<180;++attempt){
  const int fd=socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC,0);if(fd<0)return false;
  sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_ANY);address.sin_port=htons(80);
  const bool available=bind(fd,reinterpret_cast<sockaddr*>(&address),sizeof(address))==0;
  const int error=errno;close(fd);if(available)return true;if(error!=EADDRINUSE)return false;
  if(attempt%10==0)prop("sys.zkapp.state","running");usleep(500000);
 }
 return false;
}
bool start(const std::string& cfg,bool stock=false){if(!write("/tmp/owlanzi-boot.cfg",cfg))return false;prop("ctl.stop","zkswe");usleep(700000);if(stock&&!waitStockWebPort())return false;if(rename("/tmp/owlanzi-boot.cfg","/tmp/EasyUI.cfg"))return false;unlink("/tmp/owlanzi-ota-ready");prop("ctl.start","zkswe");return true;}
std::string readyText(const std::string& cfg){
 std::string expected;
 try{const auto j=nlohmann::json::parse(cfg);const auto path=j.at("startupLibPath").get<std::string>();
  for(const char* slot:{"a","b"})if(path==std::string(Root)+slot+"/lib/libzkgui.so"){
   const auto meta=nlohmann::json::parse(read((std::string(Root)+slot+"/manifest.json").c_str()));
   expected=std::string(slot)+"\n"+meta.at("version").get<std::string>()+"\n";break;
  }
 }catch(...){return {};}
 return expected;
}
bool readyFor(const std::string& cfg){const auto expected=readyText(cfg);return !expected.empty()&&read("/tmp/owlanzi-ota-ready")==expected;}
bool waitReady(const std::string& cfg){
 const auto expected=readyText(cfg);if(expected.empty())return false;
 for(int i=0;i<90;++i){if(i%10==0)prop("sys.zkapp.state","running");usleep(500000);if(read("/tmp/owlanzi-ota-ready")==expected)return true;}
 return false;
}
}
int main(int argc,char** argv){
 if(argc!=2||(std::strcmp(argv[1],"--start")&&std::strcmp(argv[1],"--stock")))return 2;
 const bool stockRequest=!std::strcmp(argv[1],"--stock");
 const bool stock=stockRequest||regular(Marker);
 std::string cfg=read(stock?Stock:"/data/owlanzi-app/current.cfg");
 if(!valid(cfg,stock)&&!stock)cfg=read("/data/owlanzi-app/previous.cfg");
 if(!valid(cfg,stock))return 3;
 int lock=open("/tmp/owlanzi-ota-switch.lock",O_CREAT|O_WRONLY|O_NOFOLLOW,0600);if(lock<0||flock(lock,LOCK_EX|LOCK_NB))return 4;
 // Set the durable selection only after validating the retained manufacturer app.
 if(stockRequest&&!write(Marker,"Selected explicitly in Owlanzi WebUI\n"))return 5;
 pid_t p=fork();if(p<0)return 6;if(p>0){close(lock);return 0;}setsid();for(int fd=0;fd<1024;++fd)if(fd!=lock)close(fd);usleep(1500000);
 prop("sys.zkapp.state","running");const bool started=start(cfg,stock);
 // Guard the manufacturer's destructive app watchdog while Owlanzi starts.
 // A failed Owlanzi boot may restore an earlier Owlanzi slot, never Ulanzi.
 if(!stock){bool ready=started&&waitReady(cfg);
  if(!ready){const auto previous=read("/data/owlanzi-app/previous.cfg");if(previous!=cfg&&valid(previous,false)&&start(previous)){cfg=previous;ready=waitReady(cfg);}}
  if(ready){write("/data/owlanzi-app/current.cfg",cfg);unlink("/data/owlanzi-app/boot-error");}
  else{
   write("/data/owlanzi-app/boot-error","Owlanzi startup failed. Keep the clock powered and contact support.\n");
   // Never let repeated app failures trigger the vendor's destructive factory
   // recovery. Keep the selected app and user data for an explicit repair.
   close(lock);
   for(;;){prop("sys.zkapp.state","running");sleep(1);if(readyFor(read("/data/owlanzi-app/current.cfg"))){unlink("/data/owlanzi-app/boot-error");return 0;}}
  }
 }
 close(lock);return started?0:8;
}
