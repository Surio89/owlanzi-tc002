// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/wifi.hpp"
#include <net/NetManager.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <chrono>
#include <thread>
#include <cstring>
#include <stdexcept>

// Exported by the installed libzknet. Used only to load the stock WLAN driver
// after WifiManager has stopped its station service; no module is replaced.
extern "C" int wifi_load_driver();
namespace owlanzi {
namespace {
using Steady=std::chrono::steady_clock;
const char* const Wpa="/data/misc/wifi/wpa_supplicant.conf";
bool exists(const std::string& p){struct stat s{};return stat(p.c_str(),&s)==0;}
template<class F> void waitFor(F predicate,int ms=8000){
 auto until=Steady::now()+std::chrono::milliseconds(ms);
 while(!predicate()){if(Steady::now()>=until)throw std::runtime_error("Wi-Fi service timeout");std::this_thread::sleep_for(std::chrono::milliseconds(100));}
}
void stopChild(pid_t& pid){
 if(pid<=0)return;
 if(waitpid(pid,nullptr,WNOHANG)==0){
  kill(pid,SIGTERM);
  for(int i=0;i<50;++i){if(waitpid(pid,nullptr,WNOHANG)!=0){pid=-1;return;}std::this_thread::sleep_for(std::chrono::milliseconds(20));}
  kill(pid,SIGKILL);waitpid(pid,nullptr,0);
 }
 pid=-1;
}
bool alive(pid_t& pid){if(pid<=0)return false;if(waitpid(pid,nullptr,WNOHANG)==0)return true;pid=-1;return false;}
pid_t launch(const char* exe,const std::string& config,bool dns){
 const auto arg="--conf-file="+config;
 const pid_t parent=getpid();pid_t pid=fork();if(pid<0)throw std::runtime_error("Could not start hotspot service");
 if(pid==0){
  prctl(PR_SET_PDEATHSIG,SIGTERM);if(getppid()!=parent)_exit(127);
  const int fd=open("/dev/null",O_RDWR);if(fd>=0){dup2(fd,0);dup2(fd,1);dup2(fd,2);if(fd>2)close(fd);}
  for(int inherited=3;inherited<1024;++inherited)close(inherited);
  if(dns)execl(exe,exe,"--keep-in-foreground",arg.c_str(),static_cast<char*>(nullptr));
  else execl(exe,exe,config.c_str(),static_cast<char*>(nullptr));
  _exit(127);
 }
 return pid;
}
void address(bool up){
 const int fd=socket(AF_INET,SOCK_DGRAM,0);if(fd<0)throw std::runtime_error("Cannot configure setup interface");
 ifreq req{};std::strcpy(req.ifr_name,"wlan0");
 auto set=[&](unsigned long op,const char* value){auto* a=reinterpret_cast<sockaddr_in*>(&req.ifr_addr);a->sin_family=AF_INET;inet_pton(AF_INET,value,&a->sin_addr);return ioctl(fd,op,&req);};
 bool ok=set(SIOCSIFADDR,up?"192.168.4.1":"0.0.0.0")==0;
 if(up){ok=ok&&set(SIOCSIFNETMASK,"255.255.255.0")==0;req.ifr_flags=IFF_UP|IFF_BROADCAST|IFF_MULTICAST;ok=ok&&ioctl(fd,SIOCSIFFLAGS,&req)==0;}
 close(fd);if(!ok&&up)throw std::runtime_error("Cannot configure setup address");
}
class NativeWifi:public WifiDriver {
 std::string directory,backup,marker;
 pid_t apPid=-1,dnsPid=-1;bool ownsHotspot=false;
 WifiManager* wifi()const{return NETMANAGER->getWifiManager();}
 void disableStation(){wifi()->enableWifi(false);waitFor([&]{return wifi()->getEnableStatus()==E_WIFI_ENABLE_DISABLE;});}
public:
 explicit NativeWifi(std::string dir):directory(std::move(dir)),backup(directory+"/wifi-rollback.conf"),marker(directory+"/wifi-rollback.json"){}
 ~NativeWifi()override{try{stopHotspot();}catch(...) {}}
 void enable()override{
  if(!wifi()->isSupported())throw std::runtime_error("Wi-Fi unsupported");
  if(!wifi()->isWifiEnable())wifi()->enableWifi(true);
  waitFor([&]{return wifi()->getEnableStatus()==E_WIFI_ENABLE_ENABLE;});
 }
 bool hasSavedNetwork()override{
  if(!exists(Wpa))return false;
  return hasSavedWifiNetwork(readPrivateFile(Wpa,65536));
 }
 void reconnect()override{
  stopHotspot();
  auto* stockAp=NETMANAGER->getSoftApManager();
  if(stockAp->isEnable()){stockAp->setEnable(false);waitFor([&]{return stockAp->getSoftApState()==E_SOFTAP_DISABLED;});}
  enable();
  // enableWifi(true) is a no-op when already enabled. Explicitly resume the
  // existing supplicant profile after the connection timeout; do not call
  // connect(ssid,password), saveConfig or change the manufacturer's WLAN file.
  const auto link=inspect();if(link.connected&&!link.ip.empty())return;
  if(link.connected){wifi()->disconnect();waitFor([&]{return !wifi()->isConnected();});}
  wifi()->reconnect();
 }
 WifiLink inspect()override{
  WifiLink result;result.supported=wifi()->isSupported();
  if(ownsHotspot){result.hotspot=alive(apPid)&&alive(dnsPid);return result;}
  result.connected=wifi()->isConnected();
  if(result.connected){if(const auto* info=wifi()->getConnectionInfo())result.ssid=info->getSsid();if(const auto* ip=wifi()->getIp())result.ip=ip;if(result.ip=="0.0.0.0")result.ip.clear();}
  return result;
 }
 std::vector<WifiNetwork> scan()override{
  if(ownsHotspot)return {};
  enable();wifi()->scan();std::this_thread::sleep_for(std::chrono::milliseconds(1800));
  std::vector<WifiInfo> infos;wifi()->getWifiScanInfosLock(infos);std::vector<WifiNetwork> out;
  for(const auto& info:infos){
   const auto& encryption=info.getEncryption();
   const bool secure=encryption.find("WPA")!=std::string::npos||encryption.find("WEP")!=std::string::npos;
   const bool supported=encryption.find("EAP")==std::string::npos&&encryption.find("WEP")==std::string::npos&&(encryption.find("SAE")==std::string::npos||encryption.find("PSK")!=std::string::npos);
   out.push_back({info.getSsid(),info.getRssi(),secure,supported});
  }
  return out;
 }
 void startHotspot()override{
  // SDK SoftApManager requires an eight-character password and hard-codes its
  // own subnet. Use the installed hostapd/dnsmasq with private Owlanzi configs
  // for an open captive portal on the same address as the TC001.
  auto* stockAp=NETMANAGER->getSoftApManager();
  if(stockAp->isEnable()){stockAp->setEnable(false);waitFor([&]{return stockAp->getSoftApState()==E_SOFTAP_DISABLED;});}
  disableStation();
  if(wifi_load_driver()!=0)throw std::runtime_error("Cannot load WLAN driver");
  waitFor([]{return if_nametoindex("wlan0")!=0;});
  ownsHotspot=true;
  try {
   address(true);
   savePrivateFile(directory+"/hostapd.conf","interface=wlan0\ndriver=nl80211\nssid=owlanzi\nchannel=6\nhw_mode=g\nignore_broadcast_ssid=0\nauth_algs=1\nwpa=0\n");
   savePrivateFile(directory+"/dnsmasq.conf","interface=wlan0\nbind-interfaces\nlisten-address=192.168.4.1\nno-resolv\nno-hosts\naddress=/#/192.168.4.1\ndhcp-range=192.168.4.20,192.168.4.100,255.255.255.0,1h\ndhcp-option=3,192.168.4.1\ndhcp-option=6,192.168.4.1\ndhcp-leasefile="+directory+"/dnsmasq.leases\npid-file="+directory+"/dnsmasq.pid\n");
   apPid=launch("/bin/hostapd",directory+"/hostapd.conf",false);
   dnsPid=launch("/bin/dnsmasq",directory+"/dnsmasq.conf",true);
   std::this_thread::sleep_for(std::chrono::milliseconds(800));
   if(!alive(apPid)||!alive(dnsPid))throw std::runtime_error("Hotspot service failed");
  }catch(...){stopHotspot();throw;}
 }
 void stopHotspot()override{if(!ownsHotspot)return;stopChild(dnsPid);stopChild(apPid);address(false);ownsHotspot=false;}
 void connect(const std::string& ssid,const std::string& password)override{
  stopHotspot();
  const bool had=exists(Wpa);
  if(had)savePrivateFile(backup,readPrivateFile(Wpa,65536));
  savePrivateFile(marker,Json({{"had_config",had}}).dump());
  enable();
  // A password change for the current SSID must authenticate again; an old
  // still-connected snapshot must not prematurely commit the new credentials.
  wifi()->disconnect();waitFor([&]{return !wifi()->isConnected();});
  wifi()->connect(ssid,password);
 }
 void commit()override{
  wifi()->saveConfig();std::this_thread::sleep_for(std::chrono::milliseconds(400));
  unlink(marker.c_str());unlink(backup.c_str());
 }
 void rollback()override{
  if(!exists(marker))return;
  stopHotspot();disableStation();
  const auto record=Json::parse(readPrivateFile(marker));
  if(record.at("had_config").get<bool>())savePrivateFile(Wpa,readPrivateFile(backup,65536));
  else unlink(Wpa);
  unlink(marker.c_str());unlink(backup.c_str());
 }
};
}
std::shared_ptr<WifiService> makeTc002Wifi(const std::string& directory){return std::make_shared<WifiService>(std::make_unique<NativeWifi>(directory));}
}
