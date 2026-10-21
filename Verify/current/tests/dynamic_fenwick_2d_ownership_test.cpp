// SPDX-License-Identifier: CC0-1.0
#ifndef FENWICK_SOURCE
#define FENWICK_SOURCE "../../../DataStructure/dynamic-binary-indexed-tree-2d.cpp"
#endif
#ifndef SMALL_POINT_LIMIT
#define SMALL_POINT_LIMIT 32
#endif
#include <cstdlib>
#include <cstdio>
#include <new>
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
static std::uint64_t allocations=0;
void* operator new(std::size_t n){++allocations;if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
[[gnu::noinline]]static void release_allocation(void*p)noexcept{std::free(p);}
void operator delete(void*p)noexcept{release_allocation(p);}
void operator delete[](void*p)noexcept{release_allocation(p);}
void operator delete(void*p,std::size_t)noexcept{release_allocation(p);}
void operator delete[](void*p,std::size_t)noexcept{release_allocation(p);}
#include FENWICK_SOURCE
using I=std::int64_t;using U=std::uint32_t;
using Row=reinsirk::detail::SmallFenwickRow<I>;
static_assert(std::is_nothrow_move_constructible_v<Row> && noexcept(std::declval<Row&>().swap(std::declval<Row&>())));
static std::uint64_t checks=0,scenarios=0;
static void require(bool b){++checks;if(!b)std::abort();}
static U common(U l,U r){return (l&r)&(U{0}-std::bit_floor(l^r));}
struct State {
    Row row;
    std::map<U,I> values;
    std::set<std::uint64_t> nodes;
    void add(U c,I v){row.add(c,v,4096);values[c]+=v;for(std::uint64_t k=c;k<=4096;k+=k&(0-k))nodes.insert(k);}
    void clear(){row.clear();values.clear();nodes.clear();}
};
static State build(int count){State s;for(int j=0;j<std::abs(count);++j)s.add(U(j*3+1),I(j%7)-3);if(count<0)s.clear();return s;}
static void verify(const Row&row,const std::map<U,I>&values,const std::set<std::uint64_t>&nodes){
    require(row.size()==nodes.size());
    if(const auto* map=row.fenwick_map()) {
        for(U c:{U(1),U(2),U(4),U(16),U(32),U(64),U(128),U(4000),U(4096)}) {
            const I* found=map->find_nonempty(c);
            if(nodes.contains(c)) {
                I expected=0;const U start=c-(c&(U(0)-c));for(auto[col,v]:values)if(start<col && col<=c)expected+=v;
                require(found && *found==expected);
            } else require(found==nullptr);
        }
    }
    I total=0;for(auto[c,v]:values){total+=v;require(row.sum(c-1,c,common(c-1,c))==v);}require(row.sum(0,4096,0)==total);
    for(auto[l,r]:{std::pair{U(0),U(4)},std::pair{U(4),U(20)},std::pair{U(20),U(120)},std::pair{U(120),U(4096)}}){I expected=0;for(auto[c,v]:values)if(l<c&&c<=r)expected+=v;require(row.sum(l,r,common(l,r))==expected);}
}
int main(){
    const int counts[]={0,1,4,5,12,24,32,33,40,-33};
    for(int a:counts)for(int b:counts){++scenarios;State left=build(a),right=build(b);verify(left.row,left.values,left.nodes);verify(right.row,right.values,right.nodes);
        auto before=allocations;left.row.swap(right.row);left.values.swap(right.values);left.nodes.swap(right.nodes);verify(left.row,left.values,left.nodes);verify(right.row,right.values,right.nodes);require(allocations==before);
        left.row.swap(right.row);left.values.swap(right.values);left.nodes.swap(right.nodes);verify(left.row,left.values,left.nodes);verify(right.row,right.values,right.nodes);require(allocations==before);
        left.row=right.row;left.values=right.values;left.nodes=right.nodes;verify(left.row,left.values,left.nodes);verify(right.row,right.values,right.nodes);
        // deep copy後の片側更新が元の所有領域を書き換えないこと。
        left.add(4000,11);verify(left.row,left.values,left.nodes);verify(right.row,right.values,right.nodes);
        before=allocations;left.row=std::move(right.row);left.values.swap(right.values);left.nodes.swap(right.nodes);right.values.clear();right.nodes.clear();require(allocations==before && right.row.empty() && right.row.fenwick_map()==nullptr);verify(left.row,left.values,left.nodes);
        before=allocations;Row moved(std::move(left.row));require(left.row.empty() && left.row.fenwick_map()==nullptr);verify(moved,left.values,left.nodes);require(allocations==before);
        Row empty;before=allocations;moved.swap(empty);verify(empty,left.values,left.nodes);require(moved.empty() && moved.fenwick_map()==nullptr && allocations==before);
    }
    Row row;for(U c=1;c<=SMALL_POINT_LIMIT;++c)row.add(c,1,4096);
    auto before=allocations;row.add(SMALL_POINT_LIMIT+1,1,4096);const auto promotion_allocations=allocations-before;
    require(promotion_allocations==3 && row.fenwick_map()!=nullptr);
    before=allocations;Row copy=row;const auto copy_allocations=allocations-before;
    require(copy_allocations==3 && copy.sum(0,4096,0)==SMALL_POINT_LIMIT+1);
    copy.add(1,7,4096);require(row.sum(0,1,0)==1 && copy.sum(0,1,0)==8);
    std::printf("{\"ownership_scenarios\":%llu,\"ownership_assertions\":%llu,\"promotion_allocations\":%llu,\"promoted_copy_allocations\":%llu,\"status\":\"pass\"}\n",static_cast<unsigned long long>(scenarios),static_cast<unsigned long long>(checks),static_cast<unsigned long long>(promotion_allocations),static_cast<unsigned long long>(copy_allocations));
}
