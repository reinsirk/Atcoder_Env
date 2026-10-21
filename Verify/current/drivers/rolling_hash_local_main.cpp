// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
int main(){
    auto check=[](bool ok){if(!ok)throw std::runtime_error("rolling hash reference mismatch");};std::mt19937_64 rng(198273);
    constexpr std::uint64_t mod=(std::uint64_t{1}<<61)-1;
    auto multiply=[](std::uint64_t a,std::uint64_t b){std::uint64_t result=0;while(b){if(b&1)result=(result+a)%mod;a=(a+a)%mod;b>>=1;}return result;};
    for(int test=0;test<160;++test){std::string s,t;for(unsigned i=0,n=rng()%45;i<n;++i)s+=static_cast<char>(rng()%256);for(unsigned i=0,n=rng()%45;i<n;++i)t+=static_cast<char>(rng()%256);
        std::uint64_t base=test%4==0?0:test%4==1?mod-1:test%4==2?1000000007:mod-2;RollingHash a(s,base),b(t,base);RollingHash compare_s(s,1000000007),compare_t(t,1000000007);
        check(a.size()==s.size());check(a.empty()==s.empty());std::uint64_t normalized=base<2?10:base>mod-2?mod-10:base;check(a.base()==normalized);
        for(std::size_t l=0;l<=s.size();++l){std::uint64_t hash=0;for(std::size_t r=l;r<=s.size();++r){check(a.get(l,r)==hash);std::size_t mid=l+(r-l)/2;check(a.concat(a.get(l,mid),a.get(mid,r),r-mid)==hash);
                if(r<s.size())hash=(multiply(hash,normalized)+static_cast<unsigned char>(s[r]))%mod;}}
        check(a.whole()==a.get(0,s.size()));
        for(int trial=0;trial<60;++trial){std::size_t l=rng()%(s.size()+1),r=l+rng()%(s.size()-l+1),ol=rng()%(t.size()+1),orr=ol+rng()%(t.size()-ol+1),limit=rng()%50;
            std::size_t common=0;while(l+common<s.size()&&ol+common<t.size()&&s[l+common]==t[ol+common]&&common<limit)++common;check(compare_s.lcp(l,compare_t,ol,limit)==common);
            check(compare_s.equal(l,r,compare_t,ol,orr)==(s.substr(l,r-l)==t.substr(ol,orr-ol)));
            auto less=[](char x,char y){return static_cast<unsigned char>(x)<static_cast<unsigned char>(y);};
            int compare=std::lexicographical_compare(s.begin()+l,s.begin()+r,t.begin()+ol,t.begin()+orr,less)?-1:std::lexicographical_compare(t.begin()+ol,t.begin()+orr,s.begin()+l,s.begin()+r,less)?1:0;
            check(compare_s.compare(l,r,compare_t,ol,orr)==compare);
        }
        auto copy=a;check(copy.whole()==a.whole());auto moved=std::move(copy);check(copy.empty());check(moved.whole()==a.whole());swap(a,b);check(a.size()==t.size());
    }std::cout<<"OK\n";
}
