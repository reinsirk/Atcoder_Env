// SPDX-License-Identifier: CC0-1.0
#ifndef FENWICK_SOURCE
#define FENWICK_SOURCE "../../../DataStructure/dynamic-binary-indexed-tree.cpp"
#endif
#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>
#include FENWICK_SOURCE
#include <cassert>
#include <random>
#include <numeric>
#include <iostream>
#include <set>
#include <type_traits>
template<class S,class T>void run(unsigned n,unsigned seed){
 std::mt19937 gen(seed);DynamicFenwickTree<S,T> bit(static_cast<S>(n));std::vector<T>a(n);std::set<unsigned> cells;
 auto check=[&]{
  assert(bit.size()==static_cast<S>(n));assert(bit.stored_nodes()==cells.size());
  auto value=[&](unsigned l,unsigned r){T sum{};for(unsigned i=l;i<r;++i)sum=static_cast<T>(sum+a[i]);return sum;};
  for(int k=0;k<30;++k){unsigned l=gen()%(n+1),r=gen()%(n+1);if(l>r)std::swap(l,r);assert(bit.sum(l,r)==value(l,r));assert(bit.sum(r)==value(0,r));if(l<n)assert(bit.get(l)==a[l]);}
  assert(bit.sum_all()==value(0,n));
  const T total=value(0,n);for(unsigned z=0;z<30;++z){T w=static_cast<T>(gen()%(static_cast<unsigned>(total)+2));unsigned lower=0,upper=0;T sum{};while(lower<n&&static_cast<T>(sum+a[lower])<w){sum=static_cast<T>(sum+a[lower]);++lower;}if(w==0)lower=0;sum=T{};while(upper<n&&static_cast<T>(sum+a[upper])<=w){sum=static_cast<T>(sum+a[upper]);++upper;}assert(bit.lower_bound(w)==static_cast<S>(lower));assert(bit.upper_bound(w)==static_cast<S>(upper));}
 };
 auto add=[&](unsigned k,T x){bit.add(k,x);a[k]=static_cast<T>(a[k]+x);for(unsigned c=k+1;c<=n;c+=c&-c)cells.insert(c);};
 // 連続追加の途中で和・探索・コピー・移動を照合する。
 for(unsigned i=0;i<n;++i){add(i,static_cast<T>(gen()%2));if((i+1)%257==0||i+1==n){check();auto copy=bit;assert(copy.stored_nodes()==bit.stored_nodes());assert(copy.sum_all()==bit.sum_all());auto moved=std::move(copy);assert(copy.empty());assert(copy.stored_nodes()==0);assert(moved.sum_all()==bit.sum_all());copy=bit;bit=std::move(copy);bit.reserve(1000);}}
 for(unsigned j=0;j<300;++j){unsigned k=gen()%n;const T target=gen()%2;bit.set(k,target);a[k]=target;for(unsigned c=k+1;c<=n;c+=c&-c)cells.insert(c);}check();
 bit.clear();std::fill(a.begin(),a.end(),T{});cells.clear();check();
 for(unsigned i=0;i<n/7;++i)add(i,static_cast<T>(gen()%2));
 check();
 bit.reset(static_cast<S>(n));std::fill(a.begin(),a.end(),T{});cells.clear();
 for(unsigned i=0;i<n/5;++i)add(i,static_cast<T>(gen()%2));
 add(n-1,T{1});check(); // 順序を変え、clear後の再利用も照合する。
 bit.reset(0);assert(bit.empty());assert(bit.sum(0)==T{});assert(bit.stored_nodes()==0);
}
int main(){for(unsigned n:{65,8191,8192,8193,16384,20001,65537}){run<int,std::int64_t>(n,n);run<std::uint64_t,std::int64_t>(n,n+7);run<unsigned,std::uint32_t>(n,n+13);}std::cout<<"OK\n";}
