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
#include <utility>
#include <vector>
using Graph=std::vector<std::vector<int>>;
static std::size_t checks=0,cases=0;
static void need(bool ok) {++checks;if(!ok){std::fprintf(stderr,"failure case %zu check %zu\n",cases,checks);std::abort();}}
static void edge(Graph&g,int u,int v){g[u].push_back(v);g[v].push_back(u);}
struct Ref {
 std::vector<int> p,d,c,r;
 Ref(const Graph&g,int root):p(g.size(),-2),d(g.size()),c(g.size()),r(g.size()) {
  int components=0;std::vector<int> q;
  for(int k=-1;k<static_cast<int>(g.size());++k){int s=k<0?root:k;if(p[s]!=-2)continue;p[s]=-1;q.clear();q.push_back(s);c[s]=components;r[s]=s;
   for(std::size_t i=0;i<q.size();++i){int v=q[i];for(int w:g[v])if(p[w]==-2){p[w]=v;d[w]=d[v]+1;c[w]=components;r[w]=s;q.push_back(w);}}
   ++components;
  }
 }
 int ancestor(int v,std::uint64_t k)const{if(k>static_cast<std::uint64_t>(d[v]))return -1;while(k--)v=p[v];return v;}
 int lca(int u,int v)const{if(c[u]!=c[v])return -1;while(d[u]>d[v])u=p[u];while(d[v]>d[u])v=p[v];while(u!=v){u=p[u];v=p[v];}return u;}
 int distance(int u,int v)const{int a=lca(u,v);return a<0?-1:(d[u]-d[a])+(d[v]-d[a]);}
 int path(int u,int v,std::uint64_t k)const{int a=lca(u,v);if(a<0)return -1;std::uint64_t x=d[u]-d[a],y=d[v]-d[a];if(k>x+y)return -1;return k<=x?ancestor(u,k):ancestor(v,x+y-k);}
 int rooted(int u,int v,int root)const{if(c[u]!=c[v]||c[u]!=c[root])return -1;int a=lca(u,v),b=lca(u,root),z=lca(v,root);return d[a]>d[b]?(d[a]>d[z]?a:z):(d[b]>d[z]?b:z);}
 bool above(int u,int v)const{return c[u]==c[v]&&d[u]<=d[v]&&ancestor(v,d[v]-d[u])==u;}
};
static void verify(const LCA&t,const Ref&r,int root){
 const int n=static_cast<int>(r.p.size());need(t.size()==r.p.size());need(t.empty()==(n==0));need(t.root()==root);int comps=0;for(int v=0;v<n;++v)if(r.p[v]<0)++comps;need(t.component_count()==static_cast<std::size_t>(comps));
 std::vector<int> seen(n);std::mt19937_64 rng(41+n);
 for(int v=0;v<n;++v){need(t.parent(v)==r.p[v]);need(t.depth_of(v)==r.d[v]);need(t.component_of(v)==r.c[v]);need(t.root_of(v)==r.r[v]);auto[a,b]=t.subtree_range(v);need(a==t.order_index(v));need(a>=0&&a<b&&b<=n);need(++seen[a]==1);need(t.subtree_size(v)==b-a);int descendants=0;
  for(int u=0;u<n;++u){bool above=r.above(v,u);descendants+=above;need(t.is_ancestor(v,u)==above);need((a<=t.order_index(u)&&t.order_index(u)<b)==above);need(t.same_component(v,u)==(r.c[v]==r.c[u]));int l=r.lca(v,u);need(t.get(v,u)==l);need(t(v,u)==l);need(t.get_distance(v,u)==r.distance(v,u));
   const auto d=r.distance(v,u);const std::uint64_t k=d<0?1:rng()%static_cast<std::uint64_t>(d+2);need(t.kth_on_path(v,u,k)==r.path(v,u,k));need(t.kth_on_path(v,u,UINT64_MAX)==-1);
  }
  need(t.subtree_size(v)==descendants);
  for(std::uint64_t k=0;k<=static_cast<std::uint64_t>(r.d[v])+1;++k)need(t.kth_ancestor(v,k)==r.ancestor(v,k));need(t.kth_ancestor(v,UINT64_MAX)==-1);
 }
 for(int k=0;k<4000;++k){int u=rng()%n,v=rng()%n,w=rng()%n;need(t.get(u,v,w)==r.rooted(u,v,w));}
}
static void run(const Graph&g,int root,bool lifecycle){++cases;Ref r(g,root);LCA t(g,root);verify(t,r,root);if(!lifecycle)return;
 LCA copy(t);verify(copy,r,root);copy=copy;copy.swap(copy);verify(copy,r,root);LCA moved(std::move(copy));need(copy.empty()&&copy.root()==-1&&copy.component_count()==0);verify(moved,r,root);copy=std::move(moved);need(moved.empty()&&moved.root()==-1);verify(copy,r,root);copy=std::move(copy);verify(copy,r,root);LCA assigned;assigned=t;verify(assigned,r,root);LCA small(Graph(1));swap(small,assigned);verify(small,r,root);need(assigned.size()==1&&assigned.parent(0)==-1);t.clear();need(t.empty()&&t.root()==-1&&t.component_count()==0);verify(copy,r,root);verify(small,r,root);t.clear();t.build(g,root);verify(t,r,root);copy.clear();verify(small,r,root);t.build(Graph{});need(t.empty()&&t.root()==-1);t.build(g,root);verify(t,r,root);
}
static Graph shape(int depth,int kind){
 const int extras=kind==0?1:kind==1?depth:kind==2?depth:depth/2;
 Graph g(depth+1+extras+4);for(int i=1;i<=depth;++i)edge(g,i-1,i);
 for(int i=0;i<extras;++i){int parent=kind==0?0:kind==1?i:kind==2?(i?depth+i:0):depth/2;edge(g,parent,depth+1+i);}return g;
}
int main(){std::mt19937 rng(29173);
 for(int h:{63,64,65,127,128,129,255,256,257,258})for(int kind=0;kind<4;++kind){Graph g=shape(h,kind);run(g,0,h==65&&kind==1);run(g,h,false);std::vector<int> perm(g.size());std::iota(perm.begin(),perm.end(),0);std::shuffle(perm.begin(),perm.end(),rng);Graph shuffled(g.size());for(int v=0;v<static_cast<int>(g.size());++v)for(int w:g[v])shuffled[perm[v]].push_back(perm[w]);for(auto&v:shuffled)std::shuffle(v.begin(),v.end(),rng);run(shuffled,perm[0],false);}
 for(int stem:{255,256,257,258}){Graph g(stem+511+4);for(int v=1;v<=stem;++v)edge(g,v-1,v);for(int i=1;i<511;++i)edge(g,stem+(i-1)/2,stem+i);run(g,0,stem==257);run(g,stem+510,false);}
 std::printf("{\"status\":\"PASS\",\"cases\":%zu,\"checks\":%zu}\n",cases,checks);
}
