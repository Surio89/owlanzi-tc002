// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>

namespace owlanzi {
// Read all 8 MiB, comparing the prepared SquashFS prefix. Unused partition
// padding need not match, but truncated/unreadable padding must still fail.
template<class ActualReader,class ExpectedReader>
bool verifyTc002ResourceReadback(std::size_t size,ActualReader actual,ExpectedReader expected){
 if(size<96||size>0x800000)return false;
 std::array<char,8192> a{},b{};
 for(std::size_t pos=0;pos<0x800000;pos+=a.size()){
  if(!actual(a.data(),a.size()))return false;
  const auto count=pos<size?std::min(a.size(),size-pos):0;
  if(count&&(!expected(b.data(),count)||std::memcmp(a.data(),b.data(),count)))return false;
 }
 return true;
}
}
