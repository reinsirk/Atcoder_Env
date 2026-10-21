// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <vector>
int main(){
    int n,q;verify_in>>n>>q;std::vector<std::vector<int>> graph(n);
    for(int i=1;i<n;++i){int p;verify_in>>p;graph[p].push_back(i);graph[i].push_back(p);}
    LCA lca(graph);
    while(q--){int u,v;verify_in>>u>>v;verify_out<<lca.get(u,v)<<'\n';}
}
