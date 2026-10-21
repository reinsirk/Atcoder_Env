// SPDX-License-Identifier: CC0-1.0
// 密な整数配列の和を、型幅ごとの独立な剰余和と照合する。
#ifndef FENWICK_SOURCE
#define FENWICK_SOURCE "../../../DataStructure/dynamic-binary-indexed-tree.cpp"
#endif
#include <algorithm>
#include <cstdlib>
#include <type_traits>
#include FENWICK_SOURCE
#include <cassert>
#include <random>
#include <cstdint>
#include <vector>
#include <iostream>
template<class T>void run(){
 using V=std::make_unsigned_t<T>;using A=decltype(V{}+0u);constexpr int n=8193;std::mt19937_64 gen(1781);DynamicFenwickTree<int,T>b(n);std::vector<T>a(n);
 for(int i=0;i<n;++i){a[i]=static_cast<T>(i%3-1);b.add(i,a[i]);}
 auto check=[&]{for(int q=0;q<1500;++q){int l=gen()%(n+1),r=gen()%(n+1);if(l>r)std::swap(l,r);if(q%11==0)l=0;if(q%13==0)r=l;if(q%17==0&&l<n)r=l+1;V total{},range{};for(int i=0;i<r;++i){total=static_cast<V>(static_cast<A>(total)+static_cast<A>(static_cast<V>(a[i])));if(i>=l)range=static_cast<V>(static_cast<A>(range)+static_cast<A>(static_cast<V>(a[i])));}assert(b.sum(l,r)==static_cast<T>(range));assert(b.sum(r)==static_cast<T>(total));}};
 check();b.clear();std::fill(a.begin(),a.end(),T{});
 for(int q=0;q<2000;++q){const int k=gen()%n;a[k]=static_cast<T>(a[k]+1);b.add(k,T{1});}check();
 auto copy=b;for(int i=0;i<n;++i)if(copy.get(i)!=a[i])std::abort();
}
int main(){run<std::int8_t>();run<std::uint8_t>();run<std::int16_t>();run<std::uint16_t>();run<std::int32_t>();run<std::uint32_t>();run<std::int64_t>();run<std::uint64_t>();std::cout<<"OK\n";}
