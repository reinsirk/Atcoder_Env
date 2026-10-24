// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_UF_REQUIRE_EXTRA
#define SIRKIT_UF_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_UF_SOURCE
#define SIRKIT_UF_SOURCE "../../../DataStructure/union_find.cpp"
#endif
#include SIRKIT_UF_SOURCE
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
std::uint64_t checks=0,states=0,operations=0;
void require(bool ok){++checks;if(!ok)throw std::runtime_error("UnionFind independent reference mismatch");}
template<class E,class F> void expect(F&& f){bool caught=false;try{f();}catch(const E&){caught=true;}require(caught);}
// 親木を使わず、結合する成分の全ラベルを書き換える参照。
struct Model {
 std::vector<int> labels;
 explicit Model(int n):labels(static_cast<std::size_t>(n)){std::iota(labels.begin(),labels.end(),0);}
 int size()const{return static_cast<int>(labels.size());}
 int root(int v)const{return labels[v];}
 int count(int v)const{return static_cast<int>(std::count(labels.begin(),labels.end(),labels[v]));}
 int components()const{int result=0;for(int i=0;i<size();++i)result+=labels[i]==i;return result;}
 bool merge(int a,int b){a=labels[a];b=labels[b];if(a==b)return false;if(count(a)<count(b))std::swap(a,b);for(auto& x:labels)if(x==b)x=a;return true;}
 std::vector<std::vector<int>> groups()const{std::vector<std::vector<int>> out(labels.size());for(int i=0;i<size();++i)out[labels[i]].push_back(i);std::erase_if(out,[](const auto& x){return x.empty();});return out;}
};
void observe(UnionFind& uf,const Model& ref,bool pairs=true){
 ++states;require(uf.size()==ref.size());require(uf.component_count()==ref.components());
#ifdef SIRKIT_UF_REQUIRE_EXTRA
 require(uf.empty()==(ref.size()==0));
#endif
 for(int i=0;i<ref.size();++i){require(uf.root(i)==ref.root(i));require(uf.leader(i)==ref.root(i));require(uf.size(i)==ref.count(i));if(pairs)for(int j=0;j<ref.size();++j){require(uf.same(i,j)==(ref.root(i)==ref.root(j)));require(uf.issame(i,j)==(ref.root(i)==ref.root(j)));}}
 require(uf.groups()==ref.groups());
}
void check_ownership(UnionFind& uf,const Model& ref){
 auto copy=uf;observe(copy,ref);if(ref.size()>1){copy.merge(0,ref.size()-1);observe(uf,ref);}
 UnionFind assigned(3);assigned=uf;observe(assigned,ref);
 auto* alias=&assigned;assigned=*alias;observe(assigned,ref);assigned=std::move(*alias);observe(assigned,ref);
 UnionFind moved(std::move(assigned));observe(moved,ref);observe(assigned,Model(0));
 UnionFind destination(7);destination=std::move(moved);observe(destination,ref);observe(moved,Model(0));
#ifdef SIRKIT_UF_REQUIRE_EXTRA
 UnionFind other(4);Model other_model(4);other.merge(0,3);other_model.merge(0,3);
 destination.swap(other);observe(destination,other_model);observe(other,ref);swap(destination,other);observe(destination,ref);observe(other,other_model);
#endif
 auto owned=uf.groups();if(!owned.empty()){owned.front().front()=-999;observe(uf,ref);}
}
void small_random(){
 std::mt19937 rng(61022);
 for(int test=0;test<64;++test){int n=test<9?test:1+static_cast<int>(rng()%32);UnionFind uf(n);Model ref(n);observe(uf,ref);
  for(int step=0;step<420;++step){if(!n)break;int a=static_cast<int>(rng()%n),b=static_cast<int>(rng()%n),kind=static_cast<int>(rng()%6);++operations;
   if(kind==0){require(uf.merge(a,b)==ref.merge(a,b));}
   else if(kind==1){uf.unite(a,b);ref.merge(a,b);}
   else if(kind==2){require(uf.same(a,b)==(ref.root(a)==ref.root(b)));require(uf.issame(a,b)==(ref.root(a)==ref.root(b)));}
   else if(kind==3){require(uf.root(a)==ref.root(a));require(uf.size(b)==ref.count(b));}
   else if(kind==4){require(!uf.merge(a,a));require(uf.same(a,a));}
   else{require(uf.groups()==ref.groups());}
   if(step%13==0)observe(uf,ref);
   if(step%137==0)check_ownership(uf,ref);
  }observe(uf,ref);check_ownership(uf,ref);
 }
}
void balanced(){
 constexpr int n=32768;
 for(bool reverse:{false,true}){UnionFind uf(n);for(int step=1;step<n;step*=2)for(int i=0;i<n;i+=2*step){int a=i,b=i+step;if(reverse)std::swap(a,b);require(uf.merge(a,b));++operations;}
  require(uf.component_count()==1);const int root=reverse?n-1:0;auto groups=uf.groups();require(groups.size()==1);require(groups[0].size()==n);for(int i=0;i<n;++i){require(groups[0][i]==i);require(uf.root(i)==root);require(uf.size(i)==n);require(uf.same(i,n-i-1));require(!uf.merge(i,root));}require(uf.component_count()==1);
 }
 UnionFind uf(8);Model ref(8);for(auto [a,b]:std::vector<std::pair<int,int>>{{7,0},{3,5},{3,4},{2,6}}){require(uf.merge(a,b)==ref.merge(a,b));}observe(uf,ref);
}
void bounds(){
 static_assert(std::is_same_v<decltype(std::declval<UnionFind&>().unite(0,0)),void>);
 static_assert(std::is_same_v<decltype(std::declval<UnionFind&>().merge(0,0)),bool>);
 const auto too_many=static_cast<std::size_t>(std::numeric_limits<int>::max())+1;
 expect<std::length_error>([&]{UnionFind invalid(too_many);(void)invalid;});
 UnionFind uf(3);Model ref(3);uf.merge(0,1);ref.merge(0,1);
#ifndef SKIP_RUNTIME_CHECKS
 for(int bad:{-1,3,std::numeric_limits<int>::max()}){
  expect<std::out_of_range>([&]{(void)uf.root(bad);});expect<std::out_of_range>([&]{(void)uf.leader(bad);});expect<std::out_of_range>([&]{(void)uf.size(bad);});
  expect<std::out_of_range>([&]{(void)uf.same(bad,bad);});expect<std::out_of_range>([&]{(void)uf.issame(bad,bad);});expect<std::out_of_range>([&]{(void)uf.merge(bad,bad);});expect<std::out_of_range>([&]{uf.unite(bad,bad);});
  for(bool left:{false,true}){int a=left?bad:0,b=left?0:bad;expect<std::out_of_range>([&]{(void)uf.same(a,b);});expect<std::out_of_range>([&]{(void)uf.merge(a,b);});expect<std::out_of_range>([&]{uf.unite(a,b);});}
 }
 UnionFind empty;expect<std::out_of_range>([&]{(void)empty.same(0,0);});expect<std::out_of_range>([&]{(void)empty.merge(0,0);});
#endif
 observe(uf,ref);
#ifdef SIRKIT_UF_REQUIRE_EXTRA
 expect<std::length_error>([&]{uf.reset(too_many);});observe(uf,ref);
#endif
}
#ifdef SIRKIT_UF_REQUIRE_EXTRA
void extra(){
 static_assert(noexcept(std::declval<UnionFind&>().clear()));static_assert(noexcept(std::declval<UnionFind&>().swap(std::declval<UnionFind&>())));
 UnionFind uf(128);for(int i=1;i<128;++i)uf.merge(i-1,i);
 for(int n:{0,1,7,64,32,256,2,0}){uf.reset(n);Model ref(n);observe(uf,ref);if(n>1){uf.merge(0,n-1);ref.merge(0,n-1);observe(uf,ref);}check_ownership(uf,ref);uf.clear();observe(uf,Model(0));uf.clear();observe(uf,Model(0));}
 uf.reset(4);uf.unite(1,3);uf.reset(4);observe(uf,Model(4));
}
#endif
}
int main(){small_random();balanced();bounds();
#ifdef SIRKIT_UF_REQUIRE_EXTRA
 extra();
#endif
 std::cout<<"PASS checks="<<checks<<" states="<<states<<" operations="<<operations<<'\n';
}
