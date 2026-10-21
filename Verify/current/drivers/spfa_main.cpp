// SPDX-License-Identifier: CC0-1.0
#include <iostream>
int main() {
    std::ios::sync_with_stdio(false); std::cin.tie(nullptr);
    int n,m,s; if (!(std::cin >> n >> m >> s)) return 0;
    WGraph graph(n);
    for (int i=0;i<m;++i) { int u,v; std::int64_t w; std::cin>>u>>v>>w; graph[u].emplace_back(v,w); }
    std::vector<std::int64_t> distance;
    if (SPFA(graph,distance,s)) { std::cout << "NEGATIVE CYCLE\n"; return 0; }
    for (auto x:distance) { if (x==INF) std::cout<<"INF\n"; else std::cout<<x<<'\n'; }
}
