// SPDX-License-Identifier: CC0-1.0
#include <atcoder/modint>
#ifndef LUCAS_HEADER
#define LUCAS_HEADER "../../../Math/Combination/Prime_Binomial.cpp"
#endif
#include LUCAS_HEADER
#include <cassert>
#include <cstdlib>
#include <new>
#include <iostream>
#include <utility>
namespace {int remaining=-1;}
void* operator new(std::size_t bytes){if(remaining==0)throw std::bad_alloc();if(remaining>0)--remaining;if(void* p=std::malloc(bytes?bytes:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t bytes){return ::operator new(bytes);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
template<class F>bool fails(int budget,F f){remaining=budget;bool failed=false;try{f();}catch(const std::bad_alloc&){failed=true;}remaining=-1;return failed;}
int main(){
    std::size_t checks=0;
    using Small=atcoder::static_modint<3>;
    for(int budget=0;budget<=2;++budget){
        PrimeBinomial<Small> source(2);auto target=std::move(source);
        const bool failed=fails(budget,[&]{source.prepare(2);});assert(failed==(budget<2));
        assert(target.Lucas(80,17).val()==2);assert(source.COM(0,0).val()==1);
        source.prepare(2);assert(source.Lucas(80,17).val()==2);++checks;
        assert(!fails(0,[&]{source.prepare(2);assert(source.Lucas(INT64_MAX,17).val()==target.Lucas(INT64_MAX,17).val());}));++checks;
    }
    using Mint=atcoder::modint998244353;
    for(int budget=0;budget<=2;++budget){
        PrimeBinomial<Mint> source(10);const auto saved=source.COM(10,3).val();
        const bool failed=fails(budget,[&]{source.prepare(8192);});assert(failed==(budget<2));
        assert(source.COM(10,3).val()==saved);source.prepare(8192);assert(source.COM(8192,2).val()==8192*8191/2);++checks;
    }
    for(int budget=0;budget<=2;++budget){
        PrimeBinomial<Mint> source(8192);auto target=std::move(source);
        const bool failed=fails(budget,[&]{source.prepare(4096);});assert(failed==(budget<2));
        assert(target.COM(8192,2).val()==8192*8191/2);
        source.prepare(0);assert(source.COM(0,0).val()==1);source.prepare(4096);assert(source.COM(4096,2).val()==4096*4095/2);++checks;
    }
    for(int budget=0;budget<=2;++budget){
        PrimeBinomial<Mint> source(8192);
        const bool failed=fails(budget,[&]{auto copy=source;assert(copy.COM(8192,2).val()==8192*8191/2);});assert(failed==(budget<2));
        assert(source.COM(8192,2).val()==8192*8191/2);++checks;
    }
    static_assert(!std::is_copy_assignable_v<PrimeBinomial<Mint>>);
    static_assert(!std::is_move_assignable_v<PrimeBinomial<Mint>>);
    std::cout<<"PASS allocation and move recovery checks="<<checks<<'\n';
}
