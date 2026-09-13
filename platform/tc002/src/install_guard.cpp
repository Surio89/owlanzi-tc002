// SPDX-License-Identifier: GPL-3.0-or-later
// The vendor updater deliberately stops WLAN. Observe completion on the clock,
// verify RES locally, then restore networking. Never write a flash partition.
#include <json.hpp>
#include "owlanzi/tc002_install_readback.hpp"
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <fcntl.h>
#include <string>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
constexpr const char* Result="/data/owlanzi-app/install-result.json";
std::string load(const char* path){
 int fd=open(path,O_RDONLY|O_NOFOLLOW);if(fd<0)return {};
 char buffer[4096];const auto n=read(fd,buffer,sizeof(buffer));close(fd);
 return n>0&&n<static_cast<ssize_t>(sizeof(buffer))?std::string(buffer,n):std::string{};
}
bool save(const char* path,const std::string& text){
 const auto next=std::string(path)+".next";
 int fd=open(next.c_str(),O_CREAT|O_WRONLY|O_TRUNC|O_NOFOLLOW,0600);if(fd<0)return false;
 std::size_t done=0;while(done<text.size()){const auto n=write(fd,text.data()+done,text.size()-done);if(n<=0){close(fd);return false;}done+=n;}
 const bool okay=fsync(fd)==0;close(fd);if(!okay||rename(next.c_str(),path))return false;
 const auto parent=std::string(path).substr(0,std::string(path).find_last_of('/'));
 fd=open(parent.c_str(),O_RDONLY|O_DIRECTORY);if(fd>=0){fsync(fd);close(fd);}return true;
}
bool prop(const char* key,const char* value){
 const auto child=fork();if(child==0){execl("/bin/setprop","setprop",key,value,static_cast<char*>(nullptr));_exit(127);}
 int status=0;return child>0&&waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0;
}
std::string property(const char* key){
 int fds[2];if(pipe(fds))return {};
 const auto child=fork();if(child==0){dup2(fds[1],STDOUT_FILENO);close(fds[0]);close(fds[1]);execl("/bin/getprop","getprop",key,static_cast<char*>(nullptr));_exit(127);}
 close(fds[1]);char bytes[128];const auto n=read(fds[0],bytes,sizeof(bytes));close(fds[0]);if(child>0)waitpid(child,nullptr,0);
 std::string value=n>0?std::string(bytes,n):std::string{};while(!value.empty()&&(value.back()=='\r'||value.back()=='\n'))value.pop_back();return value;
}
bool exact(int fd,char* bytes,std::size_t size){
 std::size_t done=0;while(done<size){const auto n=read(fd,bytes+done,size-done);if(n<=0)return false;done+=n;}return true;
}
bool verify(int expected,std::size_t size){
 const int actual=open("/dev/block/mtdblock3",O_RDONLY|O_CLOEXEC);if(actual<0)return false;
 if(lseek(expected,0,SEEK_SET)<0){close(actual);return false;}
 const bool okay=owlanzi::verifyTc002ResourceReadback(size,
  [&](char* bytes,std::size_t count){return exact(actual,bytes,count);},
  [&](char* bytes,std::size_t count){return exact(expected,bytes,count);});
 close(actual);return okay;
}
}
int main(int argc,char** argv){
 if(argc!=2||std::strcmp(argv[1],"--watch"))return 2;
 const auto nonce=load("/tmp/owlanzi-permanent-install/run-id");
 if(nonce.size()!=32||nonce.find_first_not_of("0123456789abcdef")!=std::string::npos)return 3;
 // The running trial must be an Owlanzi slot before we can resume it later.
 // zk_upgrade_ready removes startupLibPath from /tmp/EasyUI.cfg before its
 // own service restart. Retain the verified trial configuration now: reading
 // that file again after completion would resume the updater's empty UI.
 const auto resume=load("/tmp/EasyUI.cfg");bool own=false;
 try{const auto cfg=nlohmann::json::parse(resume);
  for(const char* slot:{"a","b"}){const auto root=std::string("/data/owlanzi-app/")+slot;
   if(cfg.value("startupLibPath",std::string{})==root+"/lib/libzkgui.so"&&cfg.value("resPath",std::string{})==root+"/ui/")own=true;
  }
 }catch(...){}
 if(!own)return 3;
 const int expected=open("/tmp/owlanzi-permanent-install/res.squashfs",O_RDONLY|O_NOFOLLOW|O_CLOEXEC);
 struct stat info{};char magic[4];
 if(expected<0||fstat(expected,&info)||!S_ISREG(info.st_mode)||info.st_size<96||info.st_size>0x800000||!exact(expected,magic,4)||std::memcmp(magic,"hsqs",4))return 4;
 // This Android-derived system passes its property area through an inherited
 // descriptor. getprop cannot read the upgrade status if that descriptor is
 // closed when detaching. Preserve only the validated workspace descriptor.
 const char* workspace=std::getenv("ANDROID_PROPERTY_WORKSPACE");char* end=nullptr;
 if(!workspace)return 5;
 errno=0;const long propertyFd=std::strtol(workspace,&end,10);
 if(errno||end==workspace||*end!=','||propertyFd<3||propertyFd>=1024||fcntl(static_cast<int>(propertyFd),F_GETFD)<0)return 5;
 if(property("sys.zkupgrade.state")!="-1")return 5;
 const int lock=open("/tmp/owlanzi-install.lock",O_CREAT|O_WRONLY|O_NOFOLLOW,0600);
 if(lock<0||flock(lock,LOCK_EX|LOCK_NB))return 5;
 const auto child=fork();if(child<0)return 6;if(child>0){close(lock);close(expected);return 0;}
 setsid();for(int fd=0;fd<1024;++fd)if(fd!=lock&&fd!=expected&&fd!=propertyFd)close(fd);
 // Reserve standard descriptors before opening pipes or initializing libraries.
 const int null=open("/dev/null",O_RDWR);if(null>=0){dup2(null,0);dup2(null,1);dup2(null,2);if(null>2)close(null);}
 // Verify the detached child's real access before allowing a flash trigger.
 if(property("sys.zkupgrade.state")!="-1")return 7;
 if(!save("/tmp/owlanzi-permanent-install/guard-ready",nonce))return 7;
 const auto until=std::chrono::steady_clock::now()+std::chrono::minutes(5);
 std::string phase="upgrade_timeout";
 while(std::chrono::steady_clock::now()<until){
  prop("sys.zkapp.state","running");
  const auto state=property("sys.zkupgrade.state");
  if(state=="0"){phase=verify(expected,static_cast<std::size_t>(info.st_size))?"write_verified":"readback_mismatch";break;}
  if(!state.empty()&&state!="-1"&&state!="255"){phase="upgrade_failed";break;}
  usleep(500000);
 }
 close(expected);
 const bool completed=phase=="write_verified"||phase=="readback_mismatch";
 const bool writeVerified=phase=="write_verified";
 bool remounted=false,resumeRestored=false,restartRequested=false;
 if(completed){
  // RES is no longer being written. Stop the old process before it reads more
  // resources through the pre-upgrade SquashFS mount. No software reboot.
  const bool stopped=prop("ctl.stop","zkswe")&&prop("ctl.stop","hciattach");
  const bool cleared=prop("sys.zkupgrade.flag","0")&&prop("sys.zkupgrade.dir","");
  usleep(1000000);
  // Discard the old SquashFS metadata before restarting the verified Owlanzi
  // trial. Never force an unmount while another process still uses RES.
  // The stock Bluetooth service executes /res/bin/hciattach. Stop it too;
  // Owlanzi does not use Bluetooth, and Ulanzi starts it when selected again.
  bool unmounted=false;
  if(writeVerified&&stopped&&cleared)for(int attempt=0;attempt<10;++attempt){
   if(umount("/res")==0){unmounted=true;break;}
   if(errno!=EBUSY)break;
   prop("sys.zkapp.state","running");usleep(500000);
  }
  if(writeVerified){
   if(!stopped||!cleared)phase="service_stop_failed";
   else if(!unmounted)phase="resource_unmount_failed";
   else if(mount("/dev/block/mtdblock3","/res","squashfs",MS_RDONLY,nullptr)!=0)phase="resource_mount_failed";
   else{
    remounted=true;
    resumeRestored=save("/tmp/EasyUI.cfg",resume);
    if(!resumeRestored)phase="app_config_restore_failed";
    else if(!(restartRequested=prop("ctl.start","zkswe")))phase="app_start_failed";
   }
  }
 }
 // A successful receipt now means both the byte comparison and the retained
 // app's restart preparation passed. Failures preserve their actual stage.
 save(Result,nlohmann::json{{"schema",1},{"run_id",nonce},{"phase",phase},{"verified_size",info.st_size},
  {"write_verified",writeVerified},{"resources_remounted",remounted},{"resume_restored",resumeRestored},
  {"app_restart_requested",restartRequested}}.dump()+"\n");
 // Keep the vendor's factory recovery blocked until the owner's power cycle,
 // including if remounting or the verified app's network startup failed.
 for(;;){prop("sys.zkapp.state","running");sleep(1);}
}
