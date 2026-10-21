// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <vector>
int main(){
    auto check=[](bool ok){if(!ok)throw std::runtime_error("topological reference mismatch");};
    for(int n=0;n<=4;++n)for(unsigned mask=0;mask<(1u<<(n*n));++mask){Graph g(n);for(int i=0;i<n;++i)for(int j=0;j<n;++j)if(mask>>(i*n+j)&1)g[i].push_back(j);
        std::vector<int>p(n),first;std::iota(p.begin(),p.end(),0);int count=0;
        do{std::vector<int>pos(n);for(int i=0;i<n;++i)pos[p[i]]=i;bool valid=true;for(int i=0;i<n;++i)for(int j:g[i])if(pos[i]>=pos[j])valid=false;
            if(valid){if(count==0)first=p;++count;}}while(std::next_permutation(p.begin(),p.end()));
        check(TopologicalSortLexicographical(g)==first);check(IsDAG(g)==(count!=0));
        for(bool lex:{false,true}){auto result=TopologicalSortDetails(g,lex);check(result.acyclic==(count!=0));check(result.unique==(count==1));
            if(count){check(result.order.size()==g.size());for(std::size_t i=0;i<result.order.size();++i)check(result.position[result.order[i]]==static_cast<int>(i));
                for(int i=0;i<n;++i)for(int j:g[i])check(result.position[i]<result.position[j]);if(lex)check(result.order==first);}
        }
        auto order=TopologicalSort(g);check((order.size()==g.size())==(count!=0));if(count){std::vector<int>pos(n);for(int i=0;i<n;++i)pos[order[i]]=i;for(int i=0;i<n;++i)for(int j:g[i])check(pos[i]<pos[j]);}
    }
    std::cout<<"OK\n";
}
