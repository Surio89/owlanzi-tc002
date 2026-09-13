// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "owlanzi/tc002_platform.hpp"
#include <optional>
namespace owlanzi {
// The TC002's two evdev nodes feed one encoder, as in Ulanzi KeyManager.
// Call in event timestamp order, not once per independently decoded node.
class Tc002InputDecoder {
 unsigned lastRotation=0;
 std::optional<std::uint64_t> middlePressed;
public:
 void reset(){lastRotation=0;middlePressed.reset();}
 std::optional<InputEvent> decode(unsigned type,unsigned code,int value,std::uint64_t milliseconds){
  if(type==3){ // EV_ABS: manufacturer quadrature start/end values.
   std::optional<InputEvent> result;
   if(value==0x1&&lastRotation==0x8)result=InputEvent::BrightnessUp;
   if(value==0xb&&lastRotation==0xd)result=InputEvent::BrightnessDown;
   if(value==0x8||value==0xd||value==0x1||value==0xb)lastRotation=static_cast<unsigned>(value);
   return result;
  }
  if(type!=1)return {}; // EV_KEY
  if(code==0x69&&value==0){
   const bool setup=middlePressed&&milliseconds>=*middlePressed&&milliseconds-*middlePressed>=6000;
   middlePressed.reset();if(setup)return InputEvent::WifiSetup;
  }
  if(value!=1)return {};
  if(code==0x69)middlePressed=milliseconds;
  if(code==0x67||code==0x69)return InputEvent::Ack;
  if(code==0x6c)return InputEvent::BrightnessDown;
  if(code==0x6a)return InputEvent::BrightnessUp;
  return {};
 }
};
}
