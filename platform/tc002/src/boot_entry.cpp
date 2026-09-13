// SPDX-License-Identifier: GPL-3.0-or-later
#include <unistd.h>
#include <sys/wait.h>
namespace {
void command(const char* program,const char* a,const char* b=nullptr){pid_t p=fork();if(p==0){if(b)execl(program,program,a,b,static_cast<char*>(nullptr));else execl(program,program,a,static_cast<char*>(nullptr));_exit(127);}if(p>0)waitpid(p,nullptr,0);}
}
extern "C" {
void onEasyUIInit(void*){command("/bin/setprop","sys.zkapp.state","running");command("/res/bin/owlanzi-boot-control","--start");}
void onEasyUIDeinit(void*){}
const char* onStartupApp(void*){return nullptr;}
}
