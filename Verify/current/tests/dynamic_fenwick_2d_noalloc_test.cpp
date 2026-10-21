// SPDX-License-Identifier: CC0-1.0
// このテストだけでメモリ確保関数を置き換える。
// 実行例: c++ -std=c++23 -O2 Verify/current/tests/dynamic_fenwick_2d_noalloc_test.cpp -o /tmp/fenwick_test && /tmp/fenwick_test
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
#include <climits>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <cstdlib>
#include <cstdio>
#include <new>
#include <cstddef>
#include <cstdint>
#include <tuple>
static std::uint64_t allocations=0;
void* operator new(std::size_t n) { ++allocations; if(void* p=std::malloc(n?n:1)) return p; throw std::bad_alloc(); }
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p,std::size_t) noexcept { std::free(p); }
void operator delete[](void* p,std::size_t) noexcept { std::free(p); }
#include FENWICK_SOURCE
static void require(bool b) { if(!b) std::abort(); }
int main() {
    std::uint64_t reads=0;
    for(auto [n,m] : {std::pair{65,64},std::pair{65,65},std::pair{4097,2049},std::pair{1000001,64},std::pair{1000001,65},std::pair{2147483647,2147483647}}) {
        DynamicFenwickTree2D<std::int64_t> bit(n,m);
        for(int state=0;state<3;++state) {
            if(state==1) { bit.add(0,0,7); bit.add(n-1,m-1,11); }
            if(state==2) bit.clear();
            const auto count=allocations, nodes=bit.stored_nodes();
            for(int k=0;k<100;++k) {
                require(bit.sum(0,0,n,m)==(state==1?18:0));
                require(bit.sum(n,m)==(state==1?18:0));
                require(bit.get(0,0)==(state==1?7:0));
                require(bit.get(n-1,m-1)==(state==1?11:0));
                require(bit.sum_all()==(state==1?18:0));
                require(bit.sum(n,m,n,m)==0); reads+=6;
            }
            require(allocations==count && bit.stored_nodes()==nodes);
        }
    }
    for(auto [n,m,count] : {std::tuple{1,4096,1600},std::tuple{65537,65,65},std::tuple{1000001,65,65}}) {
        DynamicFenwickTree2D<std::int64_t> bit(n,m);
        for(int j=0;j<count;++j) bit.add(n-1,j,1);
        const auto before=allocations, nodes=bit.stored_nodes();
        for(int k=0;k<100;++k) {
            require(bit.sum_all()==count); require(bit.sum(n,m)==count);
            require(bit.sum(n-1,0,n,count)==count); require(bit.get(n-1,0)==1);
            require(bit.get(n-1,count-1)==1); reads+=5;
        }
        require(allocations==before && bit.stored_nodes()==nodes);
    }
    for(int count:{1,2,3,4,5,8}) {
        constexpr int n=1000001,m=2147483647;
        DynamicFenwickTree2D<std::int64_t> bit(n,m);
        const int cols[]={0,m-1,1,m/2,2,m/2-1,m-2,3};
        for(int j=0;j<count;++j) { bit.add(n-1,cols[j],j%2?0:1); bit.add(n-1,cols[j],0); }
        const auto before=allocations,nodes=bit.stored_nodes();
        for(int k=0;k<100;++k) {
            require(bit.sum_all()==(count+1)/2);require(bit.sum(n,m)==(count+1)/2);
            for(int j=0;j<count;++j)require(bit.get(n-1,cols[j])==(j%2?0:1));
            reads+=2+count;
        }
        require(allocations==before && bit.stored_nodes()==nodes);
    }
    auto inline_buffer = [&]<class T>(std::type_identity<T>) {
        using Row=reinsirk::detail::SmallFenwickRow<T>;
        const auto before=allocations;
        Row row;
        for(int j=0;j<INLINE_POINT_LIMIT;++j) row.add(std::uint32_t(j+1),T(1),2147483647);
        require(allocations==before);
        auto copy=row;auto moved=std::move(copy);require(allocations==before && copy.empty());
        require(moved.sum(0,2147483647,0)==T(INLINE_POINT_LIMIT));
        row.clear();for(int j=0;j<INLINE_POINT_LIMIT;++j)row.add(std::uint32_t(j+1),T(0),2147483647);
        require(allocations==before);
        row.add(std::uint32_t(INLINE_POINT_LIMIT+1),T(7),2147483647);
        require(allocations>before && row.sum(0,2147483647,0)==T(7));
    };
    inline_buffer(std::type_identity<std::int64_t>{});
    inline_buffer(std::type_identity<std::uint64_t>{});
    for(int count:{16,31,32,33,40}) {
        DynamicFenwickTree2D<std::int64_t> bit(1000001,2147483647);
        for(int j=0;j<count;++j)bit.add(1000000,j*2,j%2?0:1);
        const auto before=allocations,nodes=bit.stored_nodes();
        for(int k=0;k<100;++k){require(bit.sum_all()==(count+1)/2);require(bit.get(1000000,0)==1);require(bit.get(1000000,(count-1)*2)==((count-1)%2?0:1));reads+=3;}
        require(before==allocations && bit.stored_nodes()==nodes);
    }
    std::printf("{\"read_calls\":%ju,\"allocations_during_reads\":0,\"status\":\"pass\"}\n",static_cast<std::uintmax_t>(reads));
}
