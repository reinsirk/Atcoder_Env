// SPDX-License-Identifier: CC0-1.0
#include "../../../DataStructure/implicit_treap.cpp"
#include <utility>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>
struct Affine {
 static constexpr std::int64_t mod=998244353;
 struct S {std::int64_t sum,count;};struct F {std::int64_t a,b;};
 static S op(S x,S y){return{(x.sum+y.sum)%mod,x.count+y.count};}
 static S e(){return{0,0};}
 static S mapping(F f,S x){return{(f.a*x.sum+f.b*x.count)%mod,x.count};}
 static F composition(F f,F g){return{f.a*g.a%mod,(f.a*g.b+f.b)%mod};}
 static F id(){return{1,0};}
};
using Treap=lazy_implicit_treap<Affine::S,Affine::op,Affine::e,Affine::F,Affine::mapping,Affine::composition,Affine::id,true>;
int main(){
 std::mt19937_64 rng(371912);
 std::vector<std::int64_t> ref;std::vector<Affine::S> initial;
 for(int i=0;i<2048;++i){auto x=std::int64_t(rng()%Affine::mod);ref.push_back(x);initial.push_back({x,1});}
 Treap tree(initial);
 for(int z=0;z<50000;++z){
  int n=int(ref.size()),l=int(rng()%(n+1)),r=int(rng()%(n+1));if(l>r)std::swap(l,r);
  int kind=int(rng()%5);
  if(kind==0||n==0){auto x=std::int64_t(rng()%Affine::mod);ref.insert(ref.begin()+l,x);tree.insert_at(l,{x,1});}
  else if(kind==1){int k=int(rng()%n);ref.erase(ref.begin()+k);tree.erase_at(k);}
  else if(kind==2){if(z%7==0){l=0;r=n;}else if(z%7==1)l=0;else if(z%7==2)r=n;std::reverse(ref.begin()+l,ref.begin()+r);tree.reverse(l,r);}
  else if(kind==3){auto a=std::int64_t(rng()%Affine::mod),b=std::int64_t(rng()%Affine::mod);for(int i=l;i<r;++i)ref[i]=(a*ref[i]+b)%Affine::mod;tree.apply(l,r,{a,b});}
  else{std::int64_t sum=0;for(int i=l;i<r;++i)sum=(sum+ref[i])%Affine::mod;assert(tree.prod(l,r).sum==sum);}
  assert(tree.size()==int(ref.size()));
  if(z%101==0){auto v=tree.to_vector();assert(v.size()==ref.size());std::int64_t sum=0;for(std::size_t i=0;i<ref.size();++i){assert(v[i].sum==ref[i]&&v[i].count==1);sum=(sum+ref[i])%Affine::mod;}assert(tree.all_prod().sum==sum);}
 }
 std::cout<<"50000 affine-sum operations against an independent vector passed\n";
}
