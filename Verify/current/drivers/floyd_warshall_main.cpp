// SPDX-License-Identifier: CC0-1.0
#include <iostream>
int main() {
    std::ios::sync_with_stdio(false); std::cin.tie(nullptr);
    int n,m; std::cin>>n>>m; std::vector d(n,std::vector<std::int64_t>(n,INF));
    for(int i=0;i<n;++i) d[i][i]=0;
    for(int i=0;i<m;++i) { int u,v;std::int64_t w;std::cin>>u>>v>>w;d[u][v]=std::min(d[u][v],w); }
    if(FloydWarshall(d)) { std::cout<<"NEGATIVE CYCLE\n";return 0; }
    for(const auto& row:d) { for(std::size_t j=0;j<row.size();++j) { if(j)std::cout<<' ';if(row[j]==INF)std::cout<<"INF";else std::cout<<row[j]; } std::cout<<'\n'; }
}
