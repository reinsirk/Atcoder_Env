// SPDX-License-Identifier: CC0-1.0
#ifndef LUCAS_HEADER
#define LUCAS_HEADER "../../../Math/Combination/Prime_Binomial.cpp"
#endif
#include LUCAS_HEADER
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
std::uint32_t reference(std::uint64_t n,std::uint64_t k){std::uint32_t result=1;while(k){auto a=n%3,b=k%3;if(b>a)return 0;if(a==2&&b==1)result=3-result;n/=3;k/=3;}return result;}
int main(){
    std::size_t count=0;
    for(std::uint32_t n=0;n<729;++n)for(std::uint32_t k=0;k<729;++k){assert(reinsirk::detail::lucas_mod3(n,k)==reference(n,k));++count;}
    std::mt19937_64 rng(19805);
    for(int i=0;i<100000;++i){auto n=rng(),k=rng();assert(reinsirk::detail::lucas_mod3(n,k)==reference(n,k));++count;}
    for(int i=0;i<100000;++i){auto n=rng()>>1,remaining=n;std::uint64_t k=0,place=1;while(remaining){k+=(rng()%(remaining%3+1))*place;remaining/=3;if(remaining)place*=3;}assert(reinsirk::detail::lucas_mod3(n,k)==reference(n,k));++count;}
    for(std::uint64_t n:{UINT64_C(0),UINT64_C(1),UINT64_C(242),UINT64_C(243),UINT64_C(59048),UINT64_C(59049),static_cast<std::uint64_t>(INT64_MAX),UINT64_MAX})for(std::uint64_t k:{UINT64_C(0),UINT64_C(1),UINT64_C(2),n/2,n-1,n}){assert(reinsirk::detail::lucas_mod3(n,k)==reference(n,k));++count;}
    std::cout<<"PASS ternary statistics block checks="<<count<<'\n';
}
