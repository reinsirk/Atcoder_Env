// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <iostream>
#include <numeric>
#include <queue>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>
int main(){
    auto check=[](bool ok){if(!ok)throw std::runtime_error("LCA reference mismatch");};std::mt19937 rng(182719);
    for(int test=0;test<80;++test){int n=test%41,root=n?rng()%n:0;std::vector<std::vector<int>>g(n);
        for(int i=1;i<n;++i)if(rng()%4){int p=rng()%i;g[p].push_back(i);g[i].push_back(p);}
        LCA lca(g,root);check(lca.size()==n);check(lca.empty()==(n==0));if(!n){check(lca.root()==-1);continue;}
        auto parents=[&](int r){std::vector<int>p(n,-2);std::queue<int>q;q.push(r);p[r]=-1;while(!q.empty()){int u=q.front();q.pop();for(int v:g[u])if(p[v]==-2){p[v]=u;q.push(v);}}return p;};
        std::vector<int>parent(n,-2),depth(n),roots(n);int components=0;
        for(int step=-1;step<n;++step){int r=step<0?root:step;if(parent[r]!=-2)continue;++components;auto p=parents(r);
            for(int v=0;v<n;++v)if(p[v]!=-2){parent[v]=p[v];roots[v]=r;int x=v,d=0;while(p[x]>=0){x=p[x];++d;}depth[v]=d;}}
        check(lca.component_count()==static_cast<std::size_t>(components));
        for(int u=0;u<n;++u){check(lca.parent(u)==parent[u]);check(lca.depth_of(u)==depth[u]);check(lca.root_of(u)==roots[u]);
            int ancestor=u;for(int k=0;k<=n;++k){check(lca.kth_ancestor(u,k)==ancestor);if(ancestor>=0)ancestor=parent[ancestor];}
            auto from=parents(u);
            for(int v=0;v<n;++v){int expected=-1;if(roots[u]==roots[v]){int a=u,b=v;while(depth[a]>depth[b])a=parent[a];while(depth[b]>depth[a])b=parent[b];while(a!=b){a=parent[a];b=parent[b];}expected=a;}
                check(lca.get(u,v)==expected);check(lca(u,v)==expected);check(lca.same_component(u,v)==(expected!=-1));
                check(lca.is_ancestor(u,v)==(expected==u));
                std::vector<int>path;if(from[v]!=-2){for(int x=v;x!=-1;x=from[x])path.push_back(x);std::reverse(path.begin(),path.end());}
                check(lca.get_distance(u,v)==(path.empty()?-1:static_cast<int>(path.size())-1));
                for(std::size_t k=0;k<=path.size();++k)check(lca.kth_on_path(u,v,k)==(k<path.size()?path[k]:-1));
                int new_root=(u+v)%n;auto np=parents(new_root);int answer=-1;if(np[u]!=-2&&np[v]!=-2){std::vector<bool>mark(n);for(int x=u;x!=-1;x=np[x])mark[x]=true;for(int x=v;x!=-1;x=np[x])if(mark[x]){answer=x;break;}}
                check(lca.get(u,v,new_root)==answer);
            }
        }
        auto copy=lca;lca.clear();check(lca.empty());check(copy.size()==n);auto moved=std::move(copy);check(copy.empty());check(moved.size()==n);
    }
    std::cout<<"OK\n";
}
