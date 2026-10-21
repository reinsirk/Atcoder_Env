// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
int main(){
    int n{},m{};verify_in>>n>>m;Graph graph(n);while(m--){int u{},v{};verify_in>>u>>v;graph[u-1].push_back(v-1);}
    const auto answer=TopologicalSortLexicographical(graph);
    if(answer.size()!=graph.size()){verify_out<<-1<<'\n';return 0;}
    for(std::size_t i=0;i<answer.size();++i)verify_out<<(i?" ":"")<<answer[i]+1;
    verify_out<<'\n';
}
