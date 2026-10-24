// SPDX-License-Identifier: CC0-1.0
#ifndef COMBINATION_HEADER
#define COMBINATION_HEADER "../../../Algorithm/next_combination.cpp"
#endif
#include COMBINATION_HEADER
#include COMBINATION_HEADER
#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>
#include <utility>

static_assert(next_combination(std::uint8_t{0})==0);
static_assert(next_combination(std::uint8_t{7})==11);
static_assert(next_combination(std::uint8_t{224})==0);
static_assert(next_combination(std::uint64_t{1}<<63)==0);
static_assert(first_combination<std::uint64_t>(64)==UINT64_MAX);
static_assert(last_combination<std::uint64_t>(64,1)==(UINT64_C(1)<<63));
static_assert(last_combination<std::uint8_t>(8,0)==0);

std::size_t checks=0;
template<class T>void check(int n){
    std::vector<std::vector<T>> want(n+1);
    for(std::uint32_t value=0;value<(UINT32_C(1)<<n);++value)want[std::popcount(value)].push_back(static_cast<T>(value));
    for(int k=0;k<=n;++k){
        const auto& expected=want[k];assert(first_combination<T>(k)==expected.front());assert(last_combination<T>(n,k)==expected.back());
        T mask=first_combination<T>(k);std::size_t i=0;
        do{assert(mask==expected[i++]);++checks;}while(next_combination(mask,n));
        assert(i==expected.size() && mask==expected.back());
        std::vector<T> actual;for_each_combination<T>(n,k,[&](T value){actual.push_back(value);value=0;});assert(actual==expected);++checks;
    }
}
template<class T>void width(){
    constexpr int n=std::numeric_limits<T>::digits;
    for(int k:{0,1,n-1,n}){
        std::vector<T> got;for_each_combination<T>(n,k,[&](T m){got.push_back(m);});
        assert(got.front()==first_combination<T>(k));assert(got.back()==last_combination<T>(n,k));
        assert(got.size()==static_cast<std::size_t>(k==0||k==n?1:n));
        for(T m:got)assert(std::popcount(m)==k);
        T mask=got.back();assert(!next_combination(mask,n)&&mask==got.back());
        assert(next_combination(got.back())==0);++checks;
    }
}
int main(){
    for(int n=0;n<=8;++n)check<std::uint8_t>(n);
    for(int n=0;n<=16;++n){check<std::uint16_t>(n);check<std::uint32_t>(n);check<std::uint64_t>(n);}
    width<std::uint8_t>();width<std::uint16_t>();width<std::uint32_t>();width<std::uint64_t>();
    for(int i=0;i<256;++i){int want=0;for(int j=i+1;j<256;++j)if(std::popcount(static_cast<unsigned>(i))==std::popcount(static_cast<unsigned>(j))){want=j;break;}assert(next_combination(static_cast<std::uint8_t>(i))==want);++checks;}
    assert(next_combination(INT32_MAX)==0);assert(next_combination(INT32_C(1)<<30)==0);
    bool threw=false;int visits=0;try{for_each_combination(10,4,[&](auto){if(++visits==7)throw 42;});}catch(int value){threw=value==42;}assert(threw&&visits==7);
#ifndef SKIP_RUNTIME_CHECKS
    auto rejects=[](auto fn){bool ok=false;try{fn();}catch(const std::out_of_range&){ok=true;}assert(ok);++checks;};
    rejects([]{(void)next_combination(-1);});rejects([]{(void)first_combination<std::uint8_t>(9);});rejects([]{(void)first_combination(-1);});
    rejects([]{(void)last_combination<std::uint8_t>(9,1);});rejects([]{(void)last_combination(4,5);});rejects([]{(void)last_combination(-1,0);});
    rejects([]{std::uint8_t mask=4;(void)next_combination(mask,2);});rejects([]{std::uint8_t mask=0;(void)next_combination(mask,9);});
    rejects([]{for_each_combination<std::uint8_t>(9,2,[](auto){assert(false);});});rejects([]{for_each_combination(3,4,[](auto){assert(false);});});
#endif
    std::cout<<"PASS exhaustive/boundary checks="<<checks<<'\n';
}
