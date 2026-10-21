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
static int fail_after=-1;
static std::uint64_t allocation_calls=0;
void* operator new(std::size_t n){++allocation_calls;if(fail_after==0)throw std::bad_alloc();if(fail_after>0)--fail_after;if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
[[gnu::noinline]] static void release_allocation(void*p)noexcept{std::free(p);}
void operator delete(void*p)noexcept{release_allocation(p);}
void operator delete[](void*p)noexcept{release_allocation(p);}
void operator delete(void*p,std::size_t)noexcept{release_allocation(p);}
void operator delete[](void*p,std::size_t)noexcept{release_allocation(p);}
#include FENWICK_SOURCE
using I=std::int64_t;using U=std::uint32_t;
static std::uint64_t checks=0,scenarios=0,failures=0,committed=0;
static void require(bool b){++checks;if(!b)std::abort();}
static U common(U l,U r){return (l&r)&(U{0}-std::bit_floor(l^r));}
struct PublicRow {
    DynamicFenwickTree2D<I> row{1,4096};
    void add(int col,I v){row.add(0,col,v);}
    void set(int col,I v){row.set(0,col,v);}
    I get(int col)const{return row.get(0,col);}
    I sum(int l,int r)const{return row.sum(0,l,1,r);}
    std::size_t size()const{return row.stored_nodes();}
    bool empty()const{return row.empty();}
    void clear(){row.clear();}
};
struct DirectRow {
    reinsirk::detail::SmallFenwickRow<I> row;
    void add(int col,I v){row.add(U(col+1),v,4096);}
    void set(int col,I v){add(col,v-get(col));}
    I get(int col)const{return row.sum(U(col),U(col+1),common(U(col),U(col+1)));}
    I sum(int l,int r)const{return row.sum(U(l),U(r),common(U(l),U(r)));}
    std::size_t size()const{return row.size();}
    bool empty()const{return row.empty();}
    void clear(){row.clear();}
};
static void mark(std::set<std::uint64_t>&nodes,int col){for(std::uint64_t k=std::uint64_t(col)+1;k<=4096;k+=k&(0-k))nodes.insert(k);}
template<class R>static void verify(const R&r,const std::map<int,I>&values,const std::set<std::uint64_t>&nodes){
    require(r.size()==nodes.size());I total=0;
    for(auto [c,v]:values){require(r.get(c)==v);total+=v;}
    require(r.get(201)==0 && r.get(4095)==0 && r.sum(0,4096)==total);
    for(auto [l,u]:{std::pair{0,1},std::pair{1,25},std::pair{0,65},std::pair{11,57},std::pair{0,0},std::pair{65,4096}}){I expected=0;for(auto[c,v]:values)if(l<=c&&c<u)expected+=v;require(r.sum(l,u)==expected);}
}
template<class R>static void run(){
    for(int count:{4,5,6,7,8,9,12,13,20,21,24,25,31,32})for(int position=0;position<6;++position){
        ++scenarios;R row;std::map<int,I> values;std::set<std::uint64_t> nodes;
        for(int j=0;j<count;++j){I v=j%3?I(j+1):0;row.add(j*2,v);values[j*2]=v;mark(nodes,j*2);}
        verify(row,values,nodes);const auto old_nodes=nodes;
        fail_after=position;bool threw=false;
        try{row.add(count*2,17);}catch(const std::bad_alloc&){threw=true;}
        fail_after=-1;
        if(threw){++failures;verify(row,values,old_nodes);require(row.get(count*2)==0);}
        else{++committed;values[count*2]=17;mark(nodes,count*2);}
        // 5点目失敗後は古いindex値が残る。両保持配列の更新後にも単点と範囲が一致すること。
        for(int col:{0,2,(count-1)*2}){row.add(col,3);values[col]+=3;row.add(col,0);verify(row,values,nodes);}
        row.set(0,-7);values[0]=-7;verify(row,values,nodes);
        auto copy=row;verify(copy,values,nodes);auto moved=std::move(copy);require(copy.empty());verify(moved,values,nodes);
        auto* same=&moved;moved=*same;verify(moved,values,nodes);moved=std::move(*same);verify(moved,values,nodes);
        if(threw){moved.add(count*2,17);values[count*2]=17;mark(nodes,count*2);}verify(moved,values,nodes);
        // cache/点配列を再利用し、5→6→7をもう一度通る。
        moved.clear();values.clear();nodes.clear();require(moved.size()==0 && moved.sum(0,4096)==0);
        for(int j=0;j<10;++j){moved.add(j*2,j%2?0:9);values[j*2]=j%2?0:9;mark(nodes,j*2);moved.add(0,1);++values[0];verify(moved,values,nodes);}
        const auto before=allocation_calls;
        moved.add(0,0);moved.add(8,0);verify(moved,values,nodes);
        require(allocation_calls==before);
    }
}
int main(){run<PublicRow>();run<DirectRow>();require(failures>0 && committed>0);std::printf("{\"indexed_scenarios\":%llu,\"indexed_assertions\":%llu,\"allocation_failures\":%llu,\"successful_boundaries\":%llu,\"status\":\"pass\"}\n",static_cast<unsigned long long>(scenarios),static_cast<unsigned long long>(checks),static_cast<unsigned long long>(failures),static_cast<unsigned long long>(committed));}
