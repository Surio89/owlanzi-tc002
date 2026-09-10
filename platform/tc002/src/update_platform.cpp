// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/update.hpp"
#include "owlanzi/http.hpp"
#include "owlanzi/version.hpp"
#include <curl/curl.h>
#include <mbedtls/sha256.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <array>
#include <fstream>
#include <cstring>
#include <stdexcept>
namespace owlanzi {
namespace {
const std::string Root="/data/owlanzi-app/",Origin="https://owlanzi.com/firmware/";
const std::array<std::string,3> Names={"lib/libzkgui.so","ui/main.ftu","ui/cacert.pem"};
bool exists(const std::string& p){struct stat s{};return lstat(p.c_str(),&s)==0&&S_ISREG(s.st_mode);}
std::string hash(std::istream& in,std::size_t length){
 mbedtls_sha256_context ctx;mbedtls_sha256_init(&ctx);mbedtls_sha256_starts(&ctx,0);char block[8192];
 while(length){auto n=std::min(length,sizeof(block));if(!in.read(block,n)){mbedtls_sha256_free(&ctx);throw std::runtime_error("Incomplete package");}mbedtls_sha256_update(&ctx,reinterpret_cast<unsigned char*>(block),n);length-=n;}
 unsigned char digest[32];mbedtls_sha256_finish(&ctx,digest);mbedtls_sha256_free(&ctx);constexpr char hex[]="0123456789abcdef";std::string result;for(auto b:digest){result+=hex[b>>4];result+=hex[b&15];}return result;
}
struct Transfer {int fd;std::size_t received=0,limit;std::function<void(std::size_t)> progress;std::function<bool()> cancel;};
std::size_t receive(char* p,std::size_t a,std::size_t b,void* opaque){auto& t=*static_cast<Transfer*>(opaque);const auto n=a*b;if(n>t.limit-t.received||t.cancel())return 0;std::size_t done=0;while(done<n){const auto written=write(t.fd,p+done,n-done);if(written<=0)return 0;done+=written;}t.received+=n;t.progress(t.received);return n;}
int progress(void* p,curl_off_t,curl_off_t,curl_off_t,curl_off_t){return static_cast<Transfer*>(p)->cancel()?1:0;}
void directory(const std::string& path){struct stat s{};if(mkdir(path.c_str(),0700)!=0&&(lstat(path.c_str(),&s)!=0||!S_ISDIR(s.st_mode)))throw std::runtime_error("Invalid application directory");}
class NativeUpdate:public UpdateDriver {
 std::string ca,slot,target;
 void launch(const std::string& next,const std::string& version){
  if(next!="a"&&next!="b")throw std::runtime_error("Unknown application slot");
  const auto p=fork();if(p<0)throw std::runtime_error("Cannot start updater");
  if(p==0){for(int fd=3;fd<1024;++fd)close(fd);execl((Root+"ota-switch").c_str(),"ota-switch",next.c_str(),version.c_str(),static_cast<char*>(nullptr));_exit(127);}
  int status=0;waitpid(p,&status,0);if(!WIFEXITED(status)||WEXITSTATUS(status)!=0)throw std::runtime_error("Updater did not start");
 }
public:
 NativeUpdate(std::string cert,std::string startup):ca(std::move(cert)){
  for(const auto* s:{"a","b"})if(startup==Root+s+"/lib/libzkgui.so")slot=s;
 }
 Json inspect()override{
  std::string outcome;try{outcome=readPrivateFile(Root+"result",128);}catch(...){}
  const auto other=slot=="a"?"b":"a";
  return {{"install_supported",!slot.empty()&&exists(Root+"ota-switch")},{"rollback_available",!slot.empty()&&exists(Root+other+"/manifest.json")},{"boot_mode","temporary"},{"last_result",outcome}};
 }
 Json fetchManifest(bool daily)override{
  auto transport=makeHttpsTransport(ca);HttpRequest request{"GET",updateManifestUrl(daily),"",{{"Accept-Encoding","identity"}}};
  const auto response=transport->perform(request);if(response.status!=200||response.body.size()>16384)throw std::runtime_error("Update manifest unavailable");return Json::parse(response.body);
 }
 void stage(const Release& release,std::function<void(std::size_t)> report,std::function<bool()> cancel)override{
  if(slot.empty())throw std::runtime_error("OTA loader unavailable");
  const std::string temp="/tmp/owlanzi-ota-package.bin";int fd=open(temp.c_str(),O_CREAT|O_TRUNC|O_WRONLY|O_NOFOLLOW,0600);if(fd<0)throw std::runtime_error("Cannot stage update");
  Transfer transfer{fd,0,release.size,std::move(report),std::move(cancel)};auto curl=curl_easy_init();if(!curl){close(fd);throw std::runtime_error("Download unavailable");}
  curl_easy_setopt(curl,CURLOPT_URL,(Origin+release.file).c_str());curl_easy_setopt(curl,CURLOPT_USERAGENT,"Owlanzi-TC002/" OWLANZI_APP_VERSION);
  curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2L);curl_easy_setopt(curl,CURLOPT_CAINFO,ca.c_str());
  curl_easy_setopt(curl,CURLOPT_PROTOCOLS,CURLPROTO_HTTPS);curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,0L);curl_easy_setopt(curl,CURLOPT_PROXY,"");curl_easy_setopt(curl,CURLOPT_NOSIGNAL,1L);
  curl_easy_setopt(curl,CURLOPT_ACCEPT_ENCODING,"identity");curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT,10L);curl_easy_setopt(curl,CURLOPT_TIMEOUT,180L);
  curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,receive);curl_easy_setopt(curl,CURLOPT_WRITEDATA,&transfer);curl_easy_setopt(curl,CURLOPT_NOPROGRESS,0L);curl_easy_setopt(curl,CURLOPT_XFERINFOFUNCTION,progress);curl_easy_setopt(curl,CURLOPT_XFERINFODATA,&transfer);
  auto code=curl_easy_perform(curl);long status=0;curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&status);curl_off_t length=0;curl_easy_getinfo(curl,CURLINFO_CONTENT_LENGTH_DOWNLOAD_T,&length);curl_easy_cleanup(curl);fsync(fd);close(fd);
  try{
   if(code!=CURLE_OK||status!=200||length!=static_cast<curl_off_t>(release.size)||transfer.received!=release.size||transfer.cancel())throw std::runtime_error("Download failed");
   std::ifstream input(temp,std::ios::binary);if(hash(input,release.size)!=release.sha256)throw std::runtime_error("Package checksum mismatch");input.clear();input.seekg(0);
   char magic[8];unsigned char size[4];input.read(magic,8);input.read(reinterpret_cast<char*>(size),4);
   const std::size_t headerSize=size[0]|(size[1]<<8)|(size[2]<<16)|(static_cast<std::size_t>(size[3])<<24);
   if(std::memcmp(magic,"OWLTC002",8)||headerSize<10||headerSize>16384)throw std::runtime_error("Invalid package header");
   std::string header(headerSize,'\0');input.read(&header[0],headerSize);auto meta=Json::parse(header);
   if(meta.at("version")!=release.version||meta.at("target")!="tc002"||meta.at("abi")!="z21-stock-1"||meta.at("files").size()!=3)throw std::runtime_error("Wrong package target");
   std::size_t total=12+headerSize;std::array<std::size_t,3> sizes{};std::array<std::streampos,3> offsets{};
   for(std::size_t i=0;i<3;++i){const auto& f=meta["files"][i];if(f.at("path")!=Names[i]||!f.at("size").is_number_unsigned())throw std::runtime_error("Unexpected package file");sizes[i]=f.at("size").get<std::size_t>();if(!sizes[i]||sizes[i]>release.size-total)throw std::runtime_error("Invalid file size");offsets[i]=input.tellg();if(hash(input,sizes[i])!=f.at("sha256").get<std::string>())throw std::runtime_error("File checksum mismatch");total+=sizes[i];}
   if(total!=release.size)throw std::runtime_error("Trailing package bytes");
   input.clear();input.seekg(offsets[0]);unsigned char elf[40]{};input.read(reinterpret_cast<char*>(elf),40);if(sizes[0]<40||!input||std::memcmp(elf,"\177ELF\1\1",6)||elf[16]!=3||elf[17]!=0||elf[18]!=40||elf[19]!=0||(elf[37]&4)==0)throw std::runtime_error("Not an ARM hard-float ELF32 application");
   if(transfer.cancel())throw std::runtime_error("Update cancelled");target=slot=="a"?"b":"a";const auto dir=Root+target+"/";
   directory(Root);directory(dir);directory(dir+"lib");directory(dir+"ui");
   // Only fixed inactive-slot files may be removed; never touch settings.
   for(const auto& name:Names)unlink((dir+name).c_str());unlink((dir+"manifest.json").c_str());unlink((dir+"EasyUI.cfg").c_str());
   struct statvfs space{};if(statvfs(Root.c_str(),&space)!=0||static_cast<std::uint64_t>(space.f_bavail)*space.f_frsize<release.size+524288)throw std::runtime_error("Insufficient storage");
   for(std::size_t i=0;i<3;++i){
    input.clear();input.seekg(offsets[i]);const auto path=dir+Names[i];
    const int out=open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(out<0)throw std::runtime_error("Cannot write inactive slot");
    try{std::size_t remaining=sizes[i];char buffer[8192];while(remaining){if(transfer.cancel())throw std::runtime_error("Update cancelled");const auto n=std::min(remaining,sizeof(buffer));if(!input.read(buffer,n))throw std::runtime_error("Incomplete file");std::size_t done=0;while(done<n){const auto count=write(out,buffer+done,n-done);if(count<=0)throw std::runtime_error("Storage write failed");done+=count;}remaining-=n;}if(fsync(out)!=0)throw std::runtime_error("Storage sync failed");close(out);}catch(...){close(out);throw;}
    std::ifstream stored(path,std::ios::binary);if(hash(stored,sizes[i])!=meta["files"][i].at("sha256").get<std::string>())throw std::runtime_error("Storage verification failed");
   }
   auto cfg=Json::parse(readPrivateFile("/tmp/EasyUI.cfg",16384));cfg["startupLibPath"]=dir+"lib/libzkgui.so";cfg["resPath"]=dir+"ui/";cfg["languagePath"]=dir+"tr/";
   savePrivateFile(dir+"EasyUI.cfg",cfg.dump());savePrivateFile(dir+"manifest.json",meta.dump());
   int root=open(dir.c_str(),O_RDONLY|O_DIRECTORY);if(root>=0){fsync(root);close(root);}unlink(temp.c_str());
  }catch(...){unlink(temp.c_str());throw;}
 }
 void activate(const std::string& version)override{launch(target,version);}
 void rollback()override{const std::string other=slot=="a"?"b":"a";const auto meta=Json::parse(readPrivateFile(Root+other+"/manifest.json",16384));launch(other,meta.at("version").get<std::string>());}
};
}
std::shared_ptr<UpdateService> makeTc002Updates(const std::string& data,const std::string& ca,const std::string& startup){return std::make_shared<UpdateService>(std::make_unique<NativeUpdate>(ca,startup),data);}
void tc002UpdateHeartbeat(const std::string& startup){for(const auto* slot:{"a","b"})if(startup==Root+slot+"/lib/libzkgui.so"){try{savePrivateFile("/tmp/owlanzi-ota-ready",std::string(slot)+"\n"+AppVersion+"\n");}catch(...){}break;}}
}
