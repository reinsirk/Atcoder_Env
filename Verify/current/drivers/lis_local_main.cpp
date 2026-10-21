// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>
int main(){
    auto check=[](bool ok){if(!ok)throw std::runtime_error("LIS reference mismatch");};std::mt19937 rng(123819);
    for(int test=0;test<400;++test){int n=test%11;std::vector<int>a(n);for(auto&x:a)x=static_cast<int>(rng()%7)-3;
        auto run=[&]<bool Strict>(){std::vector<std::size_t>expected(n);for(int length=1;length<=n;++length){std::size_t best=0;
                for(unsigned mask=0;mask<(1u<<length);++mask){bool ok=true;int prev=-100;std::size_t count=0;for(int i=0;i<length;++i)if(mask>>i&1){if(Strict?prev>=a[i]:prev>a[i])ok=false;prev=a[i];++count;}if(ok)best=std::max(best,count);}expected[length-1]=best;}
            check(LIS<Strict>(a)==expected);const auto length=expected.empty()?0:expected.back();check(LISLength<Strict>(a)==length);auto indices=LISIndices<Strict>(a);auto values=LISSequence<Strict>(a);check(indices.size()==length);check(values.size()==length);
            for(std::size_t j=0;j<length;++j){check(indices[j]<a.size());check(values[j]==a[indices[j]]);if(j){check(indices[j-1]<indices[j]);check(Strict?values[j-1]<values[j]:values[j-1]<=values[j]);}}
        };run.template operator()<true>();run.template operator()<false>();
    }
    std::cout<<"OK\n";
}
