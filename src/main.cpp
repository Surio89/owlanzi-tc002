// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/runtime.hpp"
#include <csignal>
#include <iostream>
#include <thread>
#include <chrono>
namespace {volatile std::sig_atomic_t stopRequested=0;void stop(int){stopRequested=1;}}
int main(int argc,char**argv) {
 try {
  owlanzi::RuntimeOptions options;int duration=0;
  for(int i=1;i<argc;++i) {
   std::string arg=argv[i];auto value=[&](){if(++i>=argc)throw std::invalid_argument("Missing argument value");return std::string(argv[i]);};
   if(arg=="--live")options.demo=false;else if(arg=="--demo")options.demo=true;
   else if(arg=="--data-dir")options.directory=value();else if(arg=="--bind")options.bind=value();
   else if(arg=="--port")options.port=std::stoi(value());else if(arg=="--ca-bundle")options.caBundle=value();
   else if(arg=="--duration")duration=std::stoi(value());
   else if(arg=="--help") {std::cout<<"Owlanzi TC002 native app (defaults: demo, loopback port 8080)\n--demo | --live --data-dir PATH --bind IP --port PORT --ca-bundle PATH\n--duration SECONDS\n";return 0;}
   else throw std::invalid_argument("Unknown argument");
  }
  if(!options.demo&&options.directory==".local/demo")options.directory=".local/live";
  if(options.demo)options.wifi=owlanzi::makeDemoWifi();
  owlanzi::Runtime runtime(options);runtime.start();
  std::cout<<"Owlanzi "<<(options.demo?"DEMO (no cloud requests)":"LIVE")<<" at http://"<<options.bind<<":"<<options.port<<"\n";
  std::cout<<"Web interface opens directly. An optional password can be set under System.\n";
  std::signal(SIGINT,stop);std::signal(SIGTERM,stop);auto begin=std::chrono::steady_clock::now();
  while(!stopRequested&&runtime.running()) {
   runtime.frame();
   if(duration>0&&std::chrono::steady_clock::now()-begin>=std::chrono::seconds(duration))break;
   std::this_thread::sleep_for(std::chrono::milliseconds(33));
  }
  runtime.stop();return 0;
 }catch(const std::exception& e){std::cerr<<"Owlanzi: "<<e.what()<<"\n";return 1;}
}
