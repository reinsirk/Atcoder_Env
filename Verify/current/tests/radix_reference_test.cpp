// SPDX-License-Identifier: CC0-1.0
#ifndef RADIX_HEADER
#define RADIX_HEADER "../../../Algorithm/Radix_sort.cpp"
#endif
#include RADIX_HEADER
#include RADIX_HEADER
#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>

std::size_t checks=0;
template<class T>void test(std::mt19937_64& rng){
    RadixSortWorkspace<T> workspace;
    for(int n:{0,1,2,31,63,64,65,66,257,1000,20000})for(int kind=0;kind<7;++kind){
        std::vector<T>a(n);
        for(int i=0;i<n;++i){auto bits=rng();a[i]=static_cast<T>(kind==1?bits%256:(kind==2?bits%17:bits));}
        if(kind==3)std::sort(a.begin(),a.end());
        if(kind==4)std::sort(a.rbegin(),a.rend());
        if(kind==5)std::fill(a.begin(),a.end(),static_cast<T>(7));
        if(kind==6&&n>=2){a[0]=std::numeric_limits<T>::min();a[1]=std::numeric_limits<T>::max();}
        auto expected=a;std::stable_sort(expected.begin(),expected.end());
        auto actual=a;auto* address=actual.data();const auto capacity=actual.capacity();workspace.sort(actual);assert(actual==expected&&actual.data()==address&&actual.capacity()==capacity);++checks;
        actual=a;RadixSort(actual);assert(actual==expected);++checks;
        actual=a;workspace.sort(actual,std::numeric_limits<T>::max());assert(actual==expected);++checks;
        actual=a;RadixSort(actual,std::numeric_limits<T>::max());assert(actual==expected);++checks;
        auto order=RadixSortOrder(a);std::vector<std::size_t> indices(n);std::iota(indices.begin(),indices.end(),0);std::stable_sort(indices.begin(),indices.end(),[&](auto i,auto j){return a[i]<a[j];});assert(order==indices);++checks;
        auto copy=workspace;actual=a;copy.sort(actual);assert(actual==expected);auto moved=std::move(copy);actual=a;moved.sort(actual);assert(actual==expected);actual=a;copy.sort(actual);assert(actual==expected);++checks;
    }
    workspace.release();assert(workspace.capacity()==0);workspace.reserve(100);assert(workspace.capacity()>=100);
#ifndef SKIP_RUNTIME_CHECKS
    for(int n:{1,64,65,1000}){
        std::vector<T>a(n,T{0});a[n-1]=std::numeric_limits<T>::max();bool rejected=false;
        try{workspace.sort(a,std::numeric_limits<T>::lowest());}catch(const std::out_of_range&){rejected=true;}
        assert(rejected);++checks;
    }
#endif
}
int main(){std::mt19937_64 rng(721038);test<std::int8_t>(rng);test<std::uint8_t>(rng);test<std::int16_t>(rng);test<std::uint16_t>(rng);test<std::int32_t>(rng);test<std::uint32_t>(rng);test<std::int64_t>(rng);test<std::uint64_t>(rng);std::cout<<"PASS std::stable_sort/reference checks="<<checks<<'\n';}
