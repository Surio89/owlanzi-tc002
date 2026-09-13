// SPDX-License-Identifier: GPL-3.0-or-later
#include "owlanzi/tc002_install_readback.hpp"
#include <cassert>
#include <vector>

bool verify(const std::vector<char>& partition,const std::vector<char>& image){
 std::size_t a=0,b=0;
 return owlanzi::verifyTc002ResourceReadback(image.size(),
  [&](char* out,std::size_t count){if(count>partition.size()-a)return false;std::memcpy(out,partition.data()+a,count);a+=count;return true;},
  [&](char* out,std::size_t count){if(count>image.size()-b)return false;std::memcpy(out,image.data()+b,count);b+=count;return true;});
}
int main(){
 std::vector<char> image(8193,'x'),partition(0x800000,'p');
 std::copy(image.begin(),image.end(),partition.begin());
 assert(verify(partition,image)); // Nonmatching unused padding is allowed.
 for(const std::size_t offset:{0u,8191u,8192u}){
  partition[offset]='!';assert(!verify(partition,image));partition[offset]='x';
 }
 partition.pop_back();assert(!verify(partition,image)); // Even beyond the image.
 assert(!verify(partition,std::vector<char>(95)));
 assert(!verify(partition,std::vector<char>(0x800001)));
 std::size_t expectedCalls=0;
 assert(!owlanzi::verifyTc002ResourceReadback(8193,
  [](char* out,std::size_t n){std::memset(out,'x',n);return true;},
  [&](char* out,std::size_t n){std::memset(out,'x',n);return ++expectedCalls<2;}));
}
