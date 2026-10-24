// SPDX-License-Identifier: CC0-1.0
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <new>
namespace {std::int64_t remaining=-1;std::size_t live_blocks=0;}
void* operator new(std::size_t bytes){if(remaining==0)throw std::bad_alloc();if(remaining>0)--remaining;if(auto* p=std::malloc(bytes?bytes:1)){++live_blocks;return p;}throw std::bad_alloc();}
void* operator new[](std::size_t bytes){return ::operator new(bytes);}
void operator delete(void* p)noexcept{if(p)--live_blocks;std::free(p);}
void operator delete[](void* p)noexcept{::operator delete(p);}
void operator delete(void* p,std::size_t)noexcept{::operator delete(p);}
void operator delete[](void* p,std::size_t)noexcept{::operator delete(p);}
#ifndef BINOMIAL_SOURCE
#define BINOMIAL_SOURCE "../../../Math/Combination/Binomial_Mod.cpp"
#endif
#include BINOMIAL_SOURCE
int main(){
    std::size_t checks=0,failures=0;
    for(std::uint32_t modulus:{1u,2u,4u,72u,65536u,1000000000u,1000000006u,1073741824u,2147483646u,4294967295u,4292870399u,4293001441u,2000000014u,3000000021u}){
        for(int bound:{0,257}){
            bool success=false;
            for(int budget=0;budget<128;++budget){
                const auto before=live_blocks;remaining=budget;bool failed=false;
                try{BinomialOptions options;options.table_limit=512;BinomialMod comb(modulus,bound,options);assert(comb.COM(12,2)==66%modulus);}
                catch(const std::bad_alloc&){failed=true;++failures;}
                remaining=-1;assert(live_blocks==before);++checks;
                if(!failed){success=true;break;}
            }
            assert(success);
            BinomialOptions options;options.table_limit=0;BinomialMod repeated(modulus,0,options);assert(repeated.COM(100,2)==4950%modulus);++checks;
        }
    }
    std::cout<<"PASS constructor allocation recovery checks="<<checks<<" failures="<<failures<<'\n';
}
