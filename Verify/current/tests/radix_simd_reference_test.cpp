// SPDX-License-Identifier: CC0-1.0
#ifndef RADIX_HEADER
#define RADIX_HEADER "../../../Algorithm/Radix_sort.cpp"
#endif
#include RADIX_HEADER
#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>

std::size_t checks=0;
template<class T>void check(const std::vector<T>& input){
    auto expected=input;std::stable_sort(expected.begin(),expected.end());
    auto actual=input;RadixSort(actual);assert(actual==expected);++checks;
    auto order=RadixSortOrder(input);std::vector<std::size_t> indices(input.size());std::iota(indices.begin(),indices.end(),0);std::stable_sort(indices.begin(),indices.end(),[&](auto a,auto b){return input[a]<input[b];});assert(order==indices);++checks;
    actual=input;RadixSort(actual,std::numeric_limits<T>::max());assert(actual==expected);++checks;
}
template<class T>void test(){
    std::mt19937_64 rng(917344);
    for(int n:{65,66,67,127,128,129,130,131,132,255,256,257,258,259,260,511,512,513,514,515,516,1023,1024,1025,1026,1027,1028}){
        std::vector<T> values(n);for(auto& x:values)x=static_cast<T>(rng());check(values);
        std::sort(values.begin(),values.end());check(values);
        // 各4要素境界の順序を一箇所だけ壊す。
        for(int pos:{1,4,5,63,64,65,124,125,126,127,128,129,252,253,254,255,256})if(pos<n){auto swapped=values;std::swap(swapped[pos-1],swapped[pos]);check(swapped);}
        std::reverse(values.begin(),values.end());check(values);std::fill(values.begin(),values.end(),T{7});check(values);
#ifndef SKIP_RUNTIME_CHECKS
        for(T maximum:std::initializer_list<T>{std::numeric_limits<T>::lowest(),T{0},static_cast<T>(std::numeric_limits<T>::max()/2),static_cast<T>(std::numeric_limits<T>::max()-1)}){
            std::vector<T> valid(n,maximum);check(valid);RadixSort(valid,maximum);++checks;
            for(int pos:{0,1,4,5,63,64,65,124,125,126,127,128,129,252,253,254,255,256,n-1})if(pos<n){
                auto invalid=valid;invalid[pos]=static_cast<T>(maximum+1);auto saved=invalid;bool rejected=false;
                try{RadixSort(invalid,maximum);}catch(const std::out_of_range&){rejected=true;}
                assert(rejected&&invalid==saved);++checks;
            }
        }
#endif
    }
}
int main(){test<std::int64_t>();test<std::uint64_t>();std::cout<<"PASS vector boundary and upper-limit checks="<<checks<<'\n';}
