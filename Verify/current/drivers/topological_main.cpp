// SPDX-License-Identifier: CC0-1.0
#include <iostream>
int main(){
    std::ios::sync_with_stdio(false);std::cin.tie(nullptr);
    int n,m;std::cin>>n>>m;Graph graph(n);while(m--){int u,v;std::cin>>u>>v;graph[u].push_back(v);}
    for(int v:TopologicalSort(graph))std::cout<<v<<'\n';
}
