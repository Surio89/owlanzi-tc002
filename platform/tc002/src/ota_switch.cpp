// SPDX-License-Identifier: GPL-3.0-or-later
// Small independent supervisor. Never writes rootfs, /res or user settings.
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
namespace {
constexpr const char* Root="/data/owlanzi-app/";
void syncRoot(){int fd=open(Root,O_RDONLY|O_DIRECTORY);if(fd>=0){fsync(fd);close(fd);}}
bool copy(const char* from,const char* to){char tmp[256];std::snprintf(tmp,sizeof(tmp),"%s.next",to);int in=open(from,O_RDONLY|O_NOFOLLOW),out=open(tmp,O_WRONLY|O_CREAT|O_TRUNC|O_NOFOLLOW,0600);bool ok=in>=0&&out>=0;char b[8192];ssize_t n=0;while(ok&&(n=read(in,b,sizeof(b)))>0){ssize_t done=0;while(done<n){auto got=write(out,b+done,n-done);if(got<=0){ok=false;break;}done+=got;}}ok=ok&&n>=0;if(in>=0)close(in);if(out>=0){ok=fsync(out)==0&&ok;close(out);}if(ok)ok=rename(tmp,to)==0;if(!ok)unlink(tmp);if(ok)syncRoot();return ok;}
void prop(const char* action,const char* value="zkswe"){pid_t p=fork();if(p==0){execl("/bin/setprop","setprop",action,value,static_cast<char*>(nullptr));_exit(127);}if(p>0)waitpid(p,nullptr,0);}
bool start(const char* cfg){prop("ctl.stop");usleep(700000);const bool ok=copy(cfg,"/tmp/EasyUI.cfg");unlink("/tmp/owlanzi-ota-ready");prop("ctl.start");return ok;}
void result(const char* text){int fd=open("/data/owlanzi-app/result.next",O_WRONLY|O_CREAT|O_TRUNC|O_NOFOLLOW,0600);if(fd>=0){const bool ok=write(fd,text,std::strlen(text))==static_cast<ssize_t>(std::strlen(text))&&fsync(fd)==0;close(fd);if(ok){rename("/data/owlanzi-app/result.next","/data/owlanzi-app/result");syncRoot();}}}
}
int main(int argc,char** argv){
 if(argc!=3||std::strlen(argv[1])!=1||(argv[1][0]!='a'&&argv[1][0]!='b')||std::strlen(argv[2])>20||std::strspn(argv[2],"0123456789.")!=std::strlen(argv[2]))return 2;
 int lock=open("/tmp/owlanzi-ota-switch.lock",O_CREAT|O_WRONLY|O_NOFOLLOW,0600);if(lock<0||flock(lock,LOCK_EX|LOCK_NB)!=0)return 3;
 pid_t p=fork();if(p<0)return 4;if(p>0){close(lock);return 0;}
 setsid();for(int fd=0;fd<1024;++fd)if(fd!=lock)close(fd);usleep(1000000);
 char cfg[256],expected[64];std::snprintf(cfg,sizeof(cfg),"%s%s/EasyUI.cfg",Root,argv[1]);std::snprintf(expected,sizeof(expected),"%s\n%s\n",argv[1],argv[2]);
 if(!copy("/tmp/EasyUI.cfg","/data/owlanzi-app/previous.cfg")){result("prepare_failed");return 5;}
 result("pending");const bool started=start(cfg);
 bool okay=false;
 // This independent supervisor owns recovery during the bounded trial; keep
 // the stock daemon from invoking its destructive factory-recovery path.
 for(int i=0;started&&i<90;++i){if(i%10==0)prop("sys.zkapp.state","running");usleep(500000);char value[64]{};int fd=open("/tmp/owlanzi-ota-ready",O_RDONLY|O_NOFOLLOW);if(fd>=0){const auto n=read(fd,value,sizeof(value)-1);close(fd);if(n>0&&!std::strcmp(value,expected)){okay=true;break;}}}
 if(okay){copy("/tmp/EasyUI.cfg","/data/owlanzi-app/current.cfg");result("confirmed");}else {start("/data/owlanzi-app/previous.cfg");copy("/data/owlanzi-app/previous.cfg","/data/owlanzi-app/current.cfg");result("rolled_back");}
 close(lock);return okay?0:6;
}
