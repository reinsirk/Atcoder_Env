// SPDX-License-Identifier: CC0-1.0
#include <cstdint>
#include <iostream>
#include <string>
int main(){
    std::ios::sync_with_stdio(false);std::cin.tie(nullptr);
    std::string text,pattern;std::cin>>text>>pattern;
    constexpr std::uint64_t base=1000000007;
    RollingHash t(text,base),p(pattern,base);
    if(pattern.size()<=text.size())for(std::size_t i=0;i<=text.size()-pattern.size();++i)
        if(t.get(i,i+pattern.size())==p.whole())std::cout<<i<<'\n';
}
