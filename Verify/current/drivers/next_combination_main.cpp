// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <vector>
int main(){
    int n{},m{};std::int64_t k{};verify_in>>n>>m>>k;
    struct E{int u{},v{};std::int64_t w{};};std::vector<E> edges(m);
    for(auto&e:edges){verify_in>>e.u>>e.v>>e.w;--e.u;--e.v;}
    std::int64_t answer=k;
    for_each_combination<std::uint32_t>(m,n-1,[&](std::uint32_t mask){
        std::array<int,8> parent;parent.fill(-1);std::int64_t sum=0;bool tree=true;
        auto root=[&](int u){while(parent[u]>=0)u=parent[u];return u;};
        while(mask){int i=std::countr_zero(mask);mask&=mask-1;const auto&e=edges[i];int u=root(e.u),v=root(e.v);
            if(u==v){tree=false;break;}parent[u]+=parent[v];parent[v]=u;sum+=e.w;
        }
        if(tree)answer=std::min(answer,sum%k);
    });verify_out<<answer<<'\n';
}
