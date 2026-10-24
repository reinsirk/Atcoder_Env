// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_RUF_REQUIRE_EXTRA
#define SIRKIT_RUF_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_RUF_SOURCE
#define SIRKIT_RUF_SOURCE "../../../DataStructure/rollback-union-find.cpp"
#endif
#include SIRKIT_RUF_SOURCE
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>
static std::size_t checks=0,operations=0,states=0;
static void require_impl(bool ok,const char*what,int line){++checks;if(!ok){std::fprintf(stderr,"line %d: %s\n",line,what);std::abort();}}
#define require(x) require_impl(bool(x),#x,__LINE__)
struct Model {
 std::vector<int> labels;std::vector<std::vector<int>> past;int snap=0;
 explicit Model(int n=0):labels(static_cast<std::size_t>(n)){std::iota(labels.begin(),labels.end(),0);}
 bool unite(int a,int b){past.push_back(labels);const int x=labels[a],y=labels[b];if(x==y)return false;for(auto&v:labels)if(v==y)v=x;return true;}
 void undo(){labels=std::move(past.back());past.pop_back();snap=std::min(snap,static_cast<int>(past.size()));}
 void rollback(int target=-1){if(target==-1)target=snap;while(static_cast<int>(past.size())>target)undo();snap=std::min(snap,target);}
 int count()const{auto v=labels;std::sort(v.begin(),v.end());return static_cast<int>(std::unique(v.begin(),v.end())-v.begin());}
 std::vector<std::vector<int>> groups()const{std::vector<std::vector<int>>r;for(std::size_t i=0;i<labels.size();++i){if(std::find(labels.begin(),labels.begin()+static_cast<std::ptrdiff_t>(i),labels[i])!=labels.begin()+static_cast<std::ptrdiff_t>(i))continue;std::vector<int>g;for(std::size_t j=i;j<labels.size();++j)if(labels[i]==labels[j])g.push_back(static_cast<int>(j));r.push_back(std::move(g));}std::sort(r.begin(),r.end());return r;}
 void clear_history(){past.clear();snap=0;}
};
static void inspect(const RollbackUnionFind&uf,const Model&m,bool full=true){
 ++states;require(uf.size()==static_cast<int>(m.labels.size()));require(uf.get_state()==static_cast<int>(m.past.size()));require(uf.component_count()==m.count());
 for(int i=0;i<uf.size();++i){const int root=uf.find(i);require(root>=0&&root<uf.size());require(uf.find(root)==root);require(uf.root(i)==root&&uf.leader(i)==root);require(m.labels[root]==m.labels[i]);const int size=static_cast<int>(std::count(m.labels.begin(),m.labels.end(),m.labels[i]));require(uf.size(i)==size);if(full)for(int j=0;j<uf.size();++j){require(uf.same(i,j)==(m.labels[i]==m.labels[j]));require(uf.issame(i,j)==uf.same(i,j));}}
 auto groups=uf.groups();for(const auto&g:groups)require(std::is_sorted(g.begin(),g.end()));std::sort(groups.begin(),groups.end());require(groups==m.groups());
}
static void merges(RollbackUnionFind&uf,Model&m,int a,int b,bool alias=false){const bool expected=m.unite(a,b);require((alias?uf.merge(a,b):uf.unite(a,b))==expected);++operations;}
static void boundary(){
 RollbackUnionFind z;Model zm;inspect(z,zm);z.snapshot();z.rollback();z.rollback(0);z.reserve_history(0);
 bool threw=false;try{RollbackUnionFind too(static_cast<std::size_t>(std::numeric_limits<int>::max())+1);(void)too;}catch(const std::length_error&){threw=true;}require(threw);
 threw=false;try{z.reserve_history(static_cast<std::size_t>(std::numeric_limits<int>::max())+1);}catch(const std::length_error&){threw=true;}require(threw);
#ifndef SKIP_RUNTIME_CHECKS
 for(int i:{-1,0,std::numeric_limits<int>::max()}){threw=false;try{(void)z.find(i);}catch(const std::out_of_range&){threw=true;}require(threw);}
 for(int which=0;which<3;++which){threw=false;try{if(which==0)(void)z.same(0,0);else if(which==1)(void)z.unite(0,0);else(void)z.merge(0,0);}catch(const std::out_of_range&){threw=true;}require(threw);}
 threw=false;try{z.undo();}catch(const std::out_of_range&){threw=true;}require(threw);
 for(int s:{-2,1,std::numeric_limits<int>::max()}){threw=false;try{z.rollback(s);}catch(const std::out_of_range&){threw=true;}require(threw);}inspect(z,zm);
 RollbackUnionFind uf(5);Model m(5);merges(uf,m,0,1);uf.snapshot();m.snap=1;
 for(int i:{-1,5,std::numeric_limits<int>::max()}){for(int which=0;which<13;++which){threw=false;try{switch(which){case 0:(void)uf.find(i);break;case 1:(void)uf.size(i);break;case 2:(void)uf.same(0,i);break;case 3:(void)uf.unite(0,i);break;case 4:(void)uf.unite(i,0);break;case 5:(void)uf.issame(i,0);break;case 6:(void)uf.merge(0,i);break;case 7:(void)uf.same(i,i);break;case 8:(void)uf.unite(i,i);break;case 9:(void)uf.merge(i,i);break;case 10:(void)uf.issame(i,i);break;case 11:(void)uf.root(i);break;default:(void)uf.leader(i);break;}}catch(const std::out_of_range&){threw=true;}require(threw);inspect(uf,m);}}
#endif
}
static void explicit_cases(){
 RollbackUnionFind uf(8);Model m(8);merges(uf,m,0,1);uf.snapshot();m.snap=1;merges(uf,m,1,2);merges(uf,m,0,2);require(uf.get_state()==3);uf.undo();m.undo();inspect(uf,m);uf.rollback();m.rollback();inspect(uf,m);
 merges(uf,m,3,4);uf.snapshot();m.snap=2;uf.undo();m.undo();uf.undo();m.undo();require(uf.get_state()==0);uf.rollback();m.rollback();inspect(uf,m);merges(uf,m,5,6);uf.rollback();m.rollback();inspect(uf,m);
 merges(uf,m,1,0);merges(uf,m,3,2);merges(uf,m,1,3);merges(uf,m,5,4);merges(uf,m,7,6);merges(uf,m,5,7);merges(uf,m,5,1);merges(uf,m,6,0);inspect(uf,m);while(uf.get_state()){uf.undo();m.undo();inspect(uf,m);}
 merges(uf,m,0,1);uf.snapshot();m.snap=1;merges(uf,m,2,3);auto copied=uf;auto cm=m;copied.undo();cm.undo();inspect(copied,cm);inspect(uf,m);copied=uf;cm=m;auto*pc=&copied;copied=*pc;inspect(copied,cm);
 RollbackUnionFind other(3);Model om(3);merges(other,om,1,2);uf.swap(other);std::swap(m,om);inspect(uf,m);inspect(other,om);
 RollbackUnionFind moved(std::move(other));auto mm=om;om=Model();inspect(other,om);inspect(moved,mm);other=std::move(moved);om=mm;mm=Model();inspect(moved,mm);inspect(other,om);auto*po=&other;other=std::move(*po);inspect(other,om);
 auto escaped=[](){RollbackUnionFind original(3);original.unite(0,2);return RollbackUnionFind(original);}();require(escaped.same(0,2));escaped.undo();require(!escaped.same(0,2));
#ifdef SIRKIT_RUF_REQUIRE_EXTRA
 static_assert(noexcept(std::declval<RollbackUnionFind&>().clear_history()));
 other.clear_history();om.clear_history();inspect(other,om);other.rollback();om.rollback();inspect(other,om);merges(other,om,0,0);other.undo();om.undo();inspect(other,om);
 RollbackUnionFind c(4);Model model(4);merges(c,model,0,1);c.snapshot();model.snap=1;merges(c,model,1,2);auto saved=c;auto sm=model;c.clear_history();model.clear_history();inspect(c,model);inspect(saved,sm);c.rollback();model.rollback();inspect(c,model);merges(c,model,2,3);c.undo();model.undo();inspect(c,model);c.clear_history();model.clear_history();inspect(c,model);
#ifndef SKIP_RUNTIME_CHECKS
 bool threw=false;try{c.rollback(1);}catch(const std::out_of_range&){threw=true;}require(threw);inspect(c,model);
#endif
#endif
}
static void noop_runs(){
 RollbackUnionFind uf(4);Model m(4);merges(uf,m,0,1);
 for(int i=0;i<5000;++i)merges(uf,m,0,1);
 uf.snapshot();m.snap=5001;merges(uf,m,2,3);
 for(int i=0;i<3000;++i)merges(uf,m,3,2);
 inspect(uf,m);uf.rollback(6000);m.rollback(6000);inspect(uf,m);uf.rollback();m.rollback();inspect(uf,m);
 uf.undo();m.undo();inspect(uf,m);merges(uf,m,1,2);uf.rollback(4999);m.rollback(4999);inspect(uf,m);
 uf.rollback(1);m.rollback(1);inspect(uf,m);uf.undo();m.undo();inspect(uf,m);uf.rollback();m.rollback();inspect(uf,m);
#ifdef SIRKIT_RUF_REQUIRE_EXTRA
 merges(uf,m,0,1);for(int i=0;i<2048;++i)merges(uf,m,0,1);uf.snapshot();m.snap=2049;uf.clear_history();m.clear_history();inspect(uf,m);uf.rollback();m.rollback();inspect(uf,m);merges(uf,m,1,2);uf.undo();m.undo();inspect(uf,m);
#endif
}
static void random_cases(){
 std::mt19937_64 rng(0x7a20261022ULL);
 for(int n:{0,1,2,3,7,16,31,32,63}){
  RollbackUnionFind uf(static_cast<std::size_t>(n));Model m(n);uf.reserve_history(256);inspect(uf,m);
  for(int iter=0;iter<1600;++iter){const auto choice=rng()%12;
   if(n&&choice<5){merges(uf,m,static_cast<int>(rng()%n),static_cast<int>(rng()%n),choice==4);}
   else if(choice==5&&!m.past.empty()){uf.undo();m.undo();++operations;}
   else if(choice==6){uf.snapshot();m.snap=static_cast<int>(m.past.size());++operations;}
   else if(choice==7){const int target=static_cast<int>(rng()%(m.past.size()+1));uf.rollback(target);m.rollback(target);++operations;}
   else if(choice==8){uf.rollback();m.rollback();++operations;}
   else if(choice==9){auto copy=uf;auto cm=m;if(n){merges(copy,cm,0,n-1);copy.rollback();cm.rollback();}inspect(copy,cm,false);inspect(uf,m,false);}
#ifdef SIRKIT_RUF_REQUIRE_EXTRA
   else if(choice==10){uf.clear_history();m.clear_history();++operations;}
#endif
   else{uf.reserve_history(static_cast<std::size_t>(iter%300));++operations;}
   inspect(uf,m,iter%31==0);
  }
  uf.rollback(0);m.rollback(0);inspect(uf,m);
 }
 RollbackUnionFind balanced(1024);Model bm(1024);for(int gap=1;gap<1024;gap*=2)for(int i=0;i<1024;i+=gap*2)merges(balanced,bm,i,i+gap);inspect(balanced,bm,false);balanced.rollback(500);bm.rollback(500);inspect(balanced,bm,false);balanced.rollback(0);bm.rollback(0);inspect(balanced,bm,false);
}
int main(){boundary();explicit_cases();noop_runs();random_cases();std::printf("PASS checks=%zu states=%zu operations=%zu\n",checks,states,operations);}
