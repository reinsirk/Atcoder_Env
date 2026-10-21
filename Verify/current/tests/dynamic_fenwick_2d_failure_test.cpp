// SPDX-License-Identifier: CC0-1.0
// このテストだけでメモリ確保関数を置き換える。
// 実行例: c++ -std=c++23 -O2 Verify/current/tests/dynamic_fenwick_2d_failure_test.cpp -o /tmp/fenwick_test && /tmp/fenwick_test
// 検査省略時は-DSKIP_RUNTIME_CHECKS、異なる保持閾値では-DSMALL_POINT_LIMIT=値を追加する。
#ifndef FENWICK_SOURCE
#define FENWICK_SOURCE "../../../DataStructure/dynamic-binary-indexed-tree-2d.cpp"
#endif
#ifndef SMALL_POINT_LIMIT
#define SMALL_POINT_LIMIT 32
#endif
#ifndef INLINE_POINT_LIMIT
#define INLINE_POINT_LIMIT 4
#endif
#include <initializer_list>
#include <utility>
#include <cstdlib>
#include <cstdio>
#include <new>
#include <cstddef>
#include <cstdint>
#include <climits>
#include <set>
static int fail_after=-1;
#if defined(__GNUC__) || defined(__clang__)
[[gnu::noinline]]
#endif
static void release_allocation(void* p) noexcept { std::free(p); }
void* operator new(std::size_t n){if(fail_after==0)throw std::bad_alloc();if(fail_after>0)--fail_after;if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void*p)noexcept{release_allocation(p);}
void operator delete[](void*p)noexcept{release_allocation(p);}
void operator delete(void*p,std::size_t)noexcept{release_allocation(p);}
void operator delete[](void*p,std::size_t)noexcept{release_allocation(p);}
#include FENWICK_SOURCE
using I=std::int64_t;
static void require(bool b){if(!b)std::abort();}
int main(){
    int failures=0,successes=0,overflow_failures=0,overflow_successes=0;
    for(int position=0;position<32;++position){
        DynamicFenwickTree2D<I> bit(1,INT_MAX);std::set<std::uint64_t> nodes;
        for(int j=0;j<SMALL_POINT_LIMIT;++j){int col=j==0?0:j==1?INT_MAX-1:j==2?1<<30:j*2;bit.add(0,col,7);for(std::uint64_t k=std::uint64_t(col)+1;k<=INT_MAX;k+=k&(0-k))nodes.insert(k);}
        require(bit.sum_all()==7*SMALL_POINT_LIMIT && bit.stored_nodes()==nodes.size());
        fail_after=position;bool threw=false;
        try{bit.add(0,3,11);}catch(const std::bad_alloc&){threw=true;}
        fail_after=-1;
        if(threw){++failures;require(bit.sum_all()==7*SMALL_POINT_LIMIT && bit.get(0,3)==0 && bit.stored_nodes()==nodes.size());bit.add(0,3,11);}
        else ++successes;
        require(bit.sum_all()==7*SMALL_POINT_LIMIT+11 && bit.get(0,3)==11);
        bit.clear();require(bit.stored_nodes()==0 && bit.sum_all()==0);bit.add(0,2,13);require(bit.get(0,2)==13 && bit.sum_all()==13);
    }
    for(int count=INLINE_POINT_LIMIT;count<SMALL_POINT_LIMIT;++count) {
        DynamicFenwickTree2D<I> bit(1,INT_MAX);std::set<std::uint64_t> nodes;
        for(int j=0;j<count;++j){bit.add(0,j*2,7);for(std::uint64_t k=std::uint64_t(j*2)+1;k<=INT_MAX;k+=k&(0-k))nodes.insert(k);}
        const auto old_nodes=nodes.size();fail_after=0;bool threw=false;
        try{bit.add(0,count*2,11);}catch(const std::bad_alloc&){threw=true;}
        fail_after=-1;
        if(threw){++overflow_failures;require(bit.sum_all()==7*count && bit.get(0,count*2)==0 && bit.stored_nodes()==old_nodes);bit.add(0,count*2,11);}
        else ++overflow_successes;
        for(std::uint64_t k=std::uint64_t(count*2)+1;k<=INT_MAX;k+=k&(0-k))nodes.insert(k);
        require(bit.sum_all()==7*count+11 && bit.get(0,count*2)==11 && bit.stored_nodes()==nodes.size());
        auto moved=std::move(bit);require(bit.empty() && moved.sum_all()==7*count+11);
    }
    require(overflow_failures>0 && overflow_successes>0);
    require(failures>0 && successes>0);
    std::printf("{\"allocation_failure_positions\":%d,\"successful_positions\":%d,\"overflow_reallocation_failures\":%d,\"overflow_noallocation_boundaries\":%d,\"status\":\"pass\"}\n",failures,successes,overflow_failures,overflow_successes);
}
