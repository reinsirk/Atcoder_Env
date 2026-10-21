// SPDX-License-Identifier: CC0-1.0
#include <cstdint>
#include <iostream>
int main() {
    std::ios::sync_with_stdio(false);std::cin.tie(nullptr);
    int n,q;std::cin>>n>>q;RollbackWeightedUnionFind<std::int64_t> uf(n);
    while(q--){int t,x,y;std::cin>>t>>x>>y;if(t==0){std::int64_t w;std::cin>>w;uf.unite(x,y,w);}
    else{const auto d=uf.try_diff(x,y);if(d)std::cout<<*d<<'\n';else std::cout<<"?\n";}}
}
