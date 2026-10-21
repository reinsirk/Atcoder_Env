// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
int main(){
    auto check=[](bool ok){if(!ok)throw std::runtime_error("combination mask mismatch");};
    for(int n=0;n<=14;++n)for(int k=0;k<=n;++k){std::vector<std::uint64_t> ref,got;
        for(std::uint64_t i=0;i<(std::uint64_t{1}<<n);++i)if(std::popcount(i)==k)ref.push_back(i);
        for_each_combination(n,k,[&](std::uint64_t x){got.push_back(x);});check(got==ref);
        auto x=first_combination<std::uint64_t>(k);std::size_t i=0;do{check(x==ref[i++]);}while(next_combination(x,n));check(i==ref.size());
    }
    std::uint64_t last=std::uint64_t{1}<<63;check(next_combination(last)==0);check(!next_combination(last,64));
    check(first_combination<std::uint64_t>(64)==std::numeric_limits<std::uint64_t>::max());check(next_combination(7)==11);check(next_combination(0)==0);
    unsigned count=0;for_each_combination(64,64,[&](std::uint64_t x){++count;check(x==~std::uint64_t{0});});check(count==1);
    std::cout<<"OK\n";
}
