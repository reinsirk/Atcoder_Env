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
static Graph arms(const std::vector<int>& lengths,int stem=0,int isolated=0){
 int n=1+stem+isolated;for(int x:lengths)n+=x;Graph g(n);int next=1,branch=0;
 for(int i=0;i<stem;++i){g[branch].push_back(next);g[next].push_back(branch);branch=next++;}
 for(int length:lengths){int p=branch;for(int i=0;i<length;++i){g[p].push_back(next);g[next].push_back(p);p=next++;}}return g;
}
static Graph merge_graphs(Graph a,const Graph&b){const int offset=static_cast<int>(a.size());a.resize(a.size()+b.size());for(int v=0;v<static_cast<int>(b.size());++v)for(int u:b[v])a[offset+v].push_back(offset+u);return a;}
static void cut_case(Graph graph,int canonical_root,bool relabel){
 ++cases;std::mt19937_64 rng(191837+cases*41);const int n=static_cast<int>(graph.size());int root=canonical_root;
 if(relabel){std::vector<int>perm(n);std::iota(perm.begin(),perm.end(),0);std::shuffle(perm.begin(),perm.end(),rng);Graph changed(n);for(int v=0;v<n;++v)for(int u:graph[v])changed[perm[v]].push_back(perm[u]);graph.swap(changed);root=perm[root];}
 for(auto&list:graph)std::shuffle(list.begin(),list.end(),rng);Ref ref(graph,root);LCA tree(graph,root);queries(tree,ref,rng,6000);
 std::vector<int>order(n);for(int v=0;v<n;++v)order[tree.order_index(v)]=v;
 for(int child=0;child<n;++child)if(ref.parent[child]==root || (ref.parent[child]>=0 && ref.parent[ref.parent[child]]==root)){auto[first,last]=tree.subtree_range(child);for(int cut:{first,last})for(int a:{cut-2,cut-1,cut,cut+1})for(int b:{cut-1,cut,cut+1,cut+2})if(a>=0&&a<n&&b>=0&&b<n){int u=order[a],v=order[b];need(tree.get(u,v)==ref.lca(u,v));need(tree.get_distance(u,v)==ref.distance(u,v));}}
 for(int i=0;i<3;++i){int changed_root=static_cast<int>(rng()%n);Ref changed(graph,changed_root);for(int j=0;j<300;++j){int u=static_cast<int>(rng()%n),v=static_cast<int>(rng()%n);int expected=ref.component[u]==ref.component[changed_root]&&ref.component[v]==ref.component[changed_root]?changed.lca(u,v):-1;need(tree.get(u,v,changed_root)==expected);}}
 LCA copy(tree);tree.clear();queries(copy,ref,rng,1000);tree.build(Graph(1));queries(copy,ref,rng,1000);swap(tree,copy);need(copy.size()==1);queries(tree,ref,rng,1000);LCA moved(std::move(tree));need(tree.empty());queries(moved,ref,rng,1000);tree.build(graph,root);moved=std::move(tree);need(tree.empty());queries(moved,ref,rng,1000);moved.clear();tree.build(graph,root);queries(tree,ref,rng,1000);
}
static Graph top_children(const std::vector<Graph>& children){Graph g(1);for(const Graph& child:children){int offset=static_cast<int>(g.size());g=merge_graphs(std::move(g),child);g[0].push_back(offset);g[offset].push_back(0);}return g;}
int main(){
 for(bool relabel:{false,true}){
  for(const auto& lengths:std::vector<std::vector<int>>{{63,63},{63,64},{64,64},{15,112},{16,111},{17,110},{1,63,63},{15,16,96},{1,1,125},{32,32,32,31},{257,257}})cut_case(arms(lengths),0,relabel);
  cut_case(arms({63,64,65},258),0,relabel);
  cut_case(arms({257,257}),257,relabel);
  cut_case(arms(std::vector<int>(128,1)),0,relabel);
  Graph forest=merge_graphs(arms({64,64}),arms({257,257}));cut_case(forest,0,relabel);cut_case(forest,129,relabel);
 }

 for(bool relabel:{false,true}){
  for(const auto& sizes:std::vector<std::pair<int,int>>{{127,127},{127,128},{128,127},{128,128},{128,129},{129,128},{129,129}}){int a=(sizes.first-1)/2,b=(sizes.second-1)/2;cut_case(top_children({arms({a,sizes.first-1-a}),arms({b,sizes.second-1-b})}),0,relabel);}
  cut_case(top_children({arms({15,112}),arms({63,64})}),0,relabel);
  cut_case(top_children({arms({16,111}),arms({63,64})}),0,relabel);
  cut_case(top_children({arms({63,64}),arms({15,112})}),0,relabel);
  cut_case(top_children({arms({1,126}),arms({1,126})}),0,relabel);
  cut_case(top_children({arms({1,63,63}),arms({32,32,32,31})}),0,relabel);
  cut_case(top_children({arms({63,64}),arms({63,64}),arms({63,64})}),0,relabel);
  cut_case(top_children({arms({257,257}),arms({63,64})}),0,relabel);
  cut_case(top_children({arms({1,513}),arms({63,64})}),0,relabel);
  cut_case(top_children({arms({63,64}),arms({257,257})}),0,relabel);
  Graph deep=top_children({arms({257,257}),arms({63,64})});cut_case(merge_graphs(deep,Graph(1)),0,relabel);cut_case(deep,1,relabel);cut_case(deep,258,relabel);
 }
 std::printf("{\"status\":\"PASS\",\"cases\":%zu,\"checks\":%zu}\n",cases,checks);
}
