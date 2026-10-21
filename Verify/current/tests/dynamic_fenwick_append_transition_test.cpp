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
#include <cstdint>
#include <vector>
#include <set>
#include <numeric>
#include <iostream>
template<class S>void test(unsigned n,unsigned p){
 DynamicFenwickTree<S,std::int64_t> b(n);std::vector<std::int64_t> v(n);std::set<unsigned>cells;
 auto track=[&](unsigned i){for(++i;i<=n;i+=i&-i)cells.insert(i);};
 for(unsigned i=0;i<p;++i){v[i]=i%7;b.add(i,v[i]);track(i);}
 auto check=[&](auto&bit){assert(bit.stored_nodes()==cells.size());std::int64_t sum=0;for(unsigned i=0;i<n;++i){assert(bit.get(i)==v[i]);assert(bit.sum(i)==sum);sum+=v[i];}assert(bit.sum_all()==sum);};
 check(b);auto c=b;check(c);DynamicFenwickTree<S,std::int64_t>d(1);d=b;check(d);b.reserve(n);check(b);
 c.add(n-1,7);v[n-1]+=7;track(n-1);check(c);
 c.set(0,99);v[0]=99;track(0);check(c);
 // コピー後の変更とclearが他の個体へ影響しないことを確かめる。
 v[n-1]=0;v[0]=0;cells.clear();for(unsigned i=0;i<p;++i)track(i);check(b);check(d);
 b.clear();assert(b.stored_nodes()==0);assert(b.sum_all()==0);for(unsigned i=0;i<1000;++i)b.add(i,1);assert(b.sum_all()==1000);
 d.reset(n+1);assert(d.stored_nodes()==0);assert(d.sum_all()==0);d.add(n,3);assert(d.get(n)==3);assert(d.sum_all()==3);
}
int main(){for(unsigned n:{8192,8193,20001,65537})for(unsigned p:{256u,1023u,1024u,1025u,4095u,4096u,4097u})if(p<n){test<int>(n,p);test<std::uint64_t>(n,p);}std::cout<<"OK\n";}
