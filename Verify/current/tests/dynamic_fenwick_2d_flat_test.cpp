// SPDX-License-Identifier: CC0-1.0
// 実行例: c++ -std=c++23 -O2 Verify/current/tests/dynamic_fenwick_2d_flat_test.cpp -o /tmp/fenwick_test && /tmp/fenwick_test
#ifndef FENWICK_SOURCE
#define FENWICK_SOURCE "../../../DataStructure/dynamic-binary-indexed-tree-2d.cpp"
#endif
// 少点・Flat128・B+・Hash間の所有権と移行失敗を独立値配列で確認する。
#include <cstddef>
#include <bit>
#include <algorithm>
#include <utility>
#include <initializer_list>
#include <cstdlib>
#include <cstdio>
#include <new>
#include <cstdint>
static std::uint64_t allocations=0;static int fail=-1;
void*operator new(std::size_t n){if(fail==0)throw std::bad_alloc();if(fail>0)--fail;++allocations;if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}void*operator new[](std::size_t n){return ::operator new(n);}
#if defined(__GNUC__) || defined(__clang__)
[[gnu::noinline]]
#endif
static void release(void*p)noexcept{std::free(p);}void operator delete(void*p)noexcept{release(p);}void operator delete[](void*p)noexcept{release(p);}void operator delete(void*p,std::size_t)noexcept{release(p);}void operator delete[](void*p,std::size_t)noexcept{release(p);}
#include FENWICK_SOURCE
#include <vector>
#include <set>
#include <random>
using I=std::int64_t;using U=std::uint32_t;using Row=reinsirk::detail::SmallFenwickRow<I>;
static std::uint64_t checks=0;static void require(bool x){++checks;if(!x)std::abort();}
static U common(U l,U r){return(l&r)&(U(0)-std::bit_floor(l^r));}
struct State{Row row;U m;std::vector<I>a;std::set<U>nodes;State(int count,U width):m(width),a(4097){for(int j=0;j<count;++j){U c=U(j+1);I v=I(j%17)-8;row.add(c,v,m);a[c]+=v;for(U k=c;k<=m;k+=k&(0-k))nodes.insert(k);}require(row.size()==nodes.size());}};
static void verify(const Row&r,const State&s){auto before=allocations;I total=0;for(U c=1;c<=4096;++c){total+=s.a[c];require(r.sum(c-1,c,common(c-1,c))==s.a[c]);}require(r.sum(0,s.m,0)==total&&r.size()==s.nodes.size());for(U l=0;l<128;l+=11)for(U end=l;end<=300;end+=23){I expected=0;for(U c=l+1;c<=end;++c)expected+=s.a[c];require(r.sum(l,end,common(l,end))==expected);}require(before==allocations);}
int main(){

 for(U ma:{U(4096),U(1000000)})for(U mb:{U(4096),U(8192)})for(int a:{0,4,32,33,64,128,129,300})for(int b:{0,4,33,64,128,129,300}){
  State left(a,ma),right(b,mb);verify(left.row,left);verify(right.row,right);auto before=allocations;left.row.swap(right.row);require(allocations==before);verify(left.row,right);verify(right.row,left);left.row.swap(right.row);Row copy=left.row;verify(copy,left);copy.add(4000,23,ma);require(left.row.sum(3999,4000,common(3999,4000))==left.a[4000]);before=allocations;Row moved(std::move(left.row));require(allocations==before&&left.row.empty());verify(moved,left);before=allocations;moved=std::move(right.row);require(allocations==before&&right.row.empty());verify(moved,right);moved.clear();require(moved.empty());for(U c=1;c<=4096;++c)require(moved.sum(c-1,c,common(c-1,c))==0);for(U c=1;c<=128;++c)moved.add(c,-3,mb);require(moved.sum(0,128,0)==-384);
 }
 // 33点/129点への移行確保失敗で元状態の値・セル数を保つ。
 for(int count:{32,128}){State src(count,1000000);fail=0;bool threw=false;try{src.row.add(U(count+1),5,src.m);}catch(const std::bad_alloc&){threw=true;}fail=-1;require(threw);verify(src.row,src);src.row.add(U(count+1),5,src.m);require(src.row.sum(U(count),U(count+1),common(U(count),U(count+1)))==5);}
 // Flatのdeepcopy失敗も代入先・元を変えない。
 {State from(64,1000000),to(4,1000000);fail=0;bool threw=false;try{to.row=from.row;}catch(const std::bad_alloc&){threw=true;}fail=-1;require(threw);verify(to.row,to);verify(from.row,from);}
 // 再利用するPayloadの未使用valueは0でなくてもsumのmaskで排除する。
 {State src(128,1000000);src.row.clear();auto before=allocations;for(U c=1;c<=64;++c)src.row.add(c,7,src.m);require(allocations==before);require(src.row.sum(0,128,0)==448&&src.row.sum(64,128,0)==0);}
 std::printf("{\"flat128_assertions\":%ju,\"row_bytes\":%zu,\"status\":\"pass\"}\n",std::uintmax_t(checks),sizeof(Row));
}
