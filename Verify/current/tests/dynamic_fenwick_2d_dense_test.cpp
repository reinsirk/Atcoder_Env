// SPDX-License-Identifier: CC0-1.0
// 実行例: c++ -std=c++23 -O2 Verify/current/tests/dynamic_fenwick_2d_dense_test.cpp -o /tmp/fenwick_test && /tmp/fenwick_test
#ifndef FENWICK_SOURCE
#define FENWICK_SOURCE "../../../DataStructure/dynamic-binary-indexed-tree-2d.cpp"
#endif
// 密化・8整数幅・確保失敗・幅変更を点列の独立参照で確認する。
#include <initializer_list>
#include <utility>
#include <type_traits>
#include <iterator>
#include <cstdlib>
#include <cstdio>
#include <new>
#include <cstdint>
#include <cstddef>
#include <climits>
#include <vector>
#include <random>
#include <algorithm>
static int fail_after=-1;static std::size_t fail_bytes=0;static std::uint64_t byte_failures=0;
void* operator new(std::size_t n){if(n==fail_bytes&&n){++byte_failures;throw std::bad_alloc();}if(fail_after==0)throw std::bad_alloc();if(fail_after>0)--fail_after;if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void*p)noexcept{std::free(p);}
void operator delete[](void*p)noexcept{std::free(p);}
void operator delete(void*p,std::size_t)noexcept{std::free(p);}
void operator delete[](void*p,std::size_t)noexcept{std::free(p);}
#include FENWICK_SOURCE
using I=std::int64_t;using U=std::uint32_t;using Row=reinsirk::detail::BPlusFenwickRow<I>;
static std::uint64_t assertions=0;static void check(bool x){++assertions;if(!x){std::fprintf(stderr,"assertion %ju failed\n",std::uintmax_t(assertions));std::abort();}}
static void run(U m){
 Row row;std::vector<I> values(m+1);std::vector<bool> used(m+1,false);std::uint64_t cells=0;std::mt19937_64 rng(193572+m);
 const U count=std::min<U>(m*2,400000);
 for(U k=0;k<count;++k){U col=U(rng()%m)+1;I delta=I(rng()%101)-50;row.add(col,delta,m);values[col]+=delta;for(U c=col;c<=m;c+=c&(U(0)-c))if(!used[c]){used[c]=true;++cells;}}
 check(row.size()==cells);for(U i=1;i<=m;++i)values[i]+=values[i-1];
 for(unsigned j=0;j<2000;++j){U l=U(rng()%(m+1)),r=U(rng()%(m+1));if(l>r)std::swap(l,r);fail_after=0;check(row.sum(l,r)==values[r]-values[l]);fail_after=-1;}
 for(U i=1;i<=m;i+=std::max<U>(1,m/1000)){fail_after=0;check(row.sum(i-1,i)==values[i]-values[i-1]);fail_after=-1;}
 Row copy=row;fail_after=0;Row moved(std::move(copy));check(copy.empty());Row target;target.swap(moved);check(moved.empty());row.clear();fail_after=-1;
 check(row.empty());for(U i=1;i<=64;++i){row.add(i,I(i)-32,m);}check(row.sum(0,64)==32);check(target.sum(0,m)==values[m]);
 row.clear(m+1);row.add(m+1,71,m+1);check(row.sum(m,m+1)==71&&row.sum(0,m)==0);row.clear(m);row.add(m,-19,m);check(row.sum(m-1,m)==-19&&row.sum(0,m-1)==0);
}
static void failure(std::size_t bytes){
 constexpr U m=8192;Row row;fail_bytes=bytes;const auto before=byte_failures;
 for(U i=1;i<=m;++i)row.add(i,I(i%7)-3,m);
 check(byte_failures==before+1);I total=0;for(U i=1;i<=m;++i){const I x=I(i%7)-3;total+=x;check(row.sum(i-1,i)==x);}check(row.sum(0,m)==total&&row.size()==m);
 row.clear();for(U i=1;i<=m;++i)row.add(i,0,m);check(byte_failures==before+2&&row.size()==m&&row.sum(0,m)==0);fail_bytes=0;
}
template<class T>static void widths(){
 using V=std::make_unsigned_t<T>;using A=decltype(V{}+0u);constexpr U m=8192;
 reinsirk::detail::BPlusFenwickRow<T> row;std::vector<V> expected(m+1);
 for(int pass=0;pass<2;++pass){
  for(U i=1;i<=m;++i){const V delta=static_cast<V>(i*197+11);row.add(i,static_cast<T>(delta),m);expected[i]=static_cast<V>(static_cast<A>(expected[i])+static_cast<A>(delta));}
  V prefix{};for(U i=1;i<=m;++i){prefix=static_cast<V>(static_cast<A>(prefix)+static_cast<A>(expected[i]));check(static_cast<V>(row.sum(i-1,i))==expected[i]);if(i%137==0)check(static_cast<V>(row.sum(0,i))==prefix);}
  check(row.size()==m);
 }
 row.clear();std::fill(expected.begin(),expected.end(),V{});
 for(U i=1;i<m;i+=37){const V delta=static_cast<V>(i*41);row.add(i,static_cast<T>(delta),m);expected[i]=delta;}
 V prefix{};for(U i=1;i<=m;++i){prefix=static_cast<V>(static_cast<A>(prefix)+static_cast<A>(expected[i]));check(static_cast<V>(row.sum(i-1,i))==expected[i]);if(i%139==0)check(static_cast<V>(row.sum(0,i))==prefix);}
}
int main(){
 widths<std::int8_t>();widths<std::uint8_t>();widths<std::int16_t>();widths<std::uint16_t>();widths<std::int32_t>();widths<std::uint32_t>();widths<std::int64_t>();widths<std::uint64_t>();
 for(U m:{U(8192),U(8193),U(16384),U(65537),U(999999),U(1000000),U(1000001)})run(m);
 failure((std::size_t(8192)+1)*sizeof(I));failure(((std::size_t(8192)+63)/64)*sizeof(std::uint64_t));
 DynamicFenwickTree2D<I> bit(1,8192);for(int i=0;i<8192;++i)bit.add(0,i,1);check(bit.sum_all()==8192&&bit.stored_nodes()==8192);
 for(int m:{8193,4096,8192,10000}){bit.reset(1,m);for(int i=0;i<m;++i)bit.add(0,i,I(i%3)-1);for(int i=0;i<m;i+=137)check(bit.get(0,i)==I(i%3)-1);check(bit.stored_nodes()==U(m));}
 std::printf("{\"assertions\":%ju,\"caught_dense_allocation_failures\":%ju,\"status\":\"pass\"}\n",std::uintmax_t(assertions),std::uintmax_t(byte_failures));
}
