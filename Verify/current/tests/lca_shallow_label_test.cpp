// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_LCA_SOURCE
#define SIRKIT_LCA_SOURCE "../../../Graph/LCA.cpp"
#endif
#include SIRKIT_LCA_SOURCE
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <random>
#include <vector>
using Graph=std::vector<std::vector<int>>;
static std::size_t checks=0,cases=0;
static void need(bool value){++checks;if(!value){std::fprintf(stderr,"failure case %zu check %zu\n",cases,checks);std::abort();}}
struct Ref {
 std::vector<int> parent,depth,component;
 Ref(const Graph&g,int root):parent(g.size(),-2),depth(g.size()),component(g.size()) {
  std::vector<int> queue;int comp=0;
  for(int it=-1;it<static_cast<int>(g.size());++it){int s=it<0?root:it;if(parent[s]!=-2)continue;parent[s]=-1;component[s]=comp++;queue.clear();queue.push_back(s);
   for(std::size_t j=0;j<queue.size();++j){int v=queue[j];for(int u:g[v])if(parent[u]==-2){parent[u]=v;depth[u]=depth[v]+1;component[u]=component[v];queue.push_back(u);}}
  }
 }
 int lca(int u,int v)const{if(component[u]!=component[v])return-1;while(depth[u]>depth[v])u=parent[u];while(depth[v]>depth[u])v=parent[v];while(u!=v){u=parent[u];v=parent[v];}return u;}
 int ancestor(int v,std::uint64_t k)const{if(k>static_cast<std::uint64_t>(depth[v]))return-1;while(k--)v=parent[v];return v;}
 int distance(int u,int v)const{int a=lca(u,v);return a<0?-1:depth[u]+depth[v]-2*depth[a];}
 int path(int u,int v,std::uint64_t k)const{int a=lca(u,v);if(a<0)return-1;std::uint64_t x=depth[u]-depth[a],y=depth[v]-depth[a];if(k>x+y)return-1;return k<=x?ancestor(u,k):ancestor(v,x+y-k);}
};
static void queries(const LCA&t,const Ref&r,std::mt19937_64&rng,int count){
 const int n=static_cast<int>(r.parent.size());
 for(int i=0;i<count;++i){int u=static_cast<int>(rng()%n),v=static_cast<int>(rng()%n);const int a=r.lca(u,v),d=r.distance(u,v);need(t.get(u,v)==a);need(t(v,u)==a);need(t.get_distance(u,v)==d);need(t.same_component(u,v)==(r.component[u]==r.component[v]));need(t.parent(u)==r.parent[u]);need(t.depth_of(u)==r.depth[u]);need(t.component_of(u)==r.component[u]);const std::uint64_t k=rng()%static_cast<unsigned int>(r.depth[u]+2);need(t.kth_ancestor(u,k)==r.ancestor(u,k));need(t.kth_ancestor(u,UINT64_MAX)==-1);const std::uint64_t step=d<0?0:rng()%static_cast<unsigned int>(d+2);need(t.kth_on_path(u,v,step)==r.path(u,v,step));need(t.kth_on_path(u,v,UINT64_MAX)==-1);bool above=a==u;need(t.is_ancestor(u,v)==above);auto[first,last]=t.subtree_range(u);need(t.order_index(u)==first);need(t.subtree_size(u)==last-first);need((first<=t.order_index(v)&&t.order_index(v)<last)==above);}
}
static void run(int n,int kind,int root_choice){
 ++cases;std::mt19937_64 rng(0x8175e129ULL+static_cast<unsigned int>(n)*97+kind*13+root_choice);Graph graph(n);std::vector<int> perm(n);std::iota(perm.begin(),perm.end(),0);std::shuffle(perm.begin(),perm.end(),rng);
 for(int v=1;v<n;++v){if(kind==2&&v%97==0)continue;int p=kind==1?static_cast<int>(rng()%static_cast<unsigned int>(v)):(v-1)/2;graph[perm[v]].push_back(perm[p]);graph[perm[p]].push_back(perm[v]);}
 for(auto&adj:graph)std::shuffle(adj.begin(),adj.end(),rng);int root=perm[root_choice? n-1:0];Ref ref(graph,root);LCA tree(graph,root);need(tree.size()==static_cast<std::size_t>(n));queries(tree,ref,rng,20000);
 for(int i=0;i<4;++i){int new_root=static_cast<int>(rng()%n);Ref changed(graph,new_root);for(int j=0;j<500;++j){int u=static_cast<int>(rng()%n),v=static_cast<int>(rng()%n);int expected=ref.component[u]==ref.component[new_root]&&ref.component[v]==ref.component[new_root]?changed.lca(u,v):-1;need(tree.get(u,v,new_root)==expected);}}
 LCA copy(tree);tree.clear();queries(copy,ref,rng,1000);tree.build(Graph(1));queries(copy,ref,rng,1000);LCA moved(std::move(copy));need(copy.empty()&&copy.root()==-1);queries(moved,ref,rng,1000);copy.build(graph,root);moved.clear();queries(copy,ref,rng,1000);
}
int main(){for(int n:{31,32,33,63,64,65,127,128,129,255,256,257,4095,4096,4097,131071,131072,131073})for(int kind=0;kind<3;++kind)for(int root=0;root<2;++root)run(n,kind,root);std::printf("{\"status\":\"PASS\",\"cases\":%zu,\"checks\":%zu}\n",cases,checks);}
