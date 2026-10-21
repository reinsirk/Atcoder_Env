// SPDX-License-Identifier: CC0-1.0
// 内部の行実装と共通部品の併用を検証するテスト。
// 実行例: c++ -std=c++23 -O2 Verify/current/tests/dynamic_fenwick_2d_rows_test.cpp -o /tmp/fenwick_test && /tmp/fenwick_test
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
#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <utility>
#include <vector>
#include FENWICK_SOURCE
#include "../../../DataStructure/dynamic-binary-indexed-tree.cpp"
#include "../../../DataStructure/Hash_map.cpp"
#include FENWICK_SOURCE
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <random>
#include <set>
using U=std::uint32_t;
using I=std::int64_t;
static std::uint64_t checks=0;
static void require(bool ok){++checks;if(!ok)std::abort();}
static void mark(std::set<std::uint64_t>& nodes,U c,U m){for(std::uint64_t k=c;k<=m;k+=k&(0-k))nodes.insert(k);}
static U common(U l,U r){return (l&r)&(U{0}-std::bit_floor(l^r));}
static void verify(const reinsirk::detail::SmallFenwickRow<I>& row,const std::map<U,I>& values,const std::set<std::uint64_t>& nodes,U m){
    require(row.size()==nodes.size());
    const U ends[]={0,1,m/2,m-1,m};
    for(U l:ends)for(U r:ends)if(l<=r){I ref=0;for(auto [c,v]:values)if(l<c&&c<=r)ref+=v;require(row.sum(l,r,common(l,r))==ref);}
}
int main(){
    for(U m:{U(1),U(2),U(3),U(7),U(8),U(15),U(16),U(31),U(32),U(63),U(64),U(65),U(127),U(128),U(129),U(INT_MAX)}){
        std::vector<U> cols;
        if(m<=129){for(U c=1;c<=m;++c)cols.push_back(c);}else cols={1,2,3,4,63,64,65,1023,1024,1025,U(1)<<30,(U(1)<<30)+1,m-1,m};
        for(U a:cols)for(U b:cols){
            reinsirk::detail::SmallFenwickRow<I> row;std::map<U,I> values;std::set<std::uint64_t> nodes;
            for(auto [c,v]:{std::pair{a,I(0)},std::pair{b,I(3)},std::pair{a,I(9)},std::pair{b,I(-3)},std::pair{m,I(5)},std::pair{U(1),I(7)}}){
                row.add(c,v,m);values[c]+=v;mark(nodes,c,m);verify(row,values,nodes,m);
            }
            auto copy=row;verify(copy,values,nodes,m);auto moved=std::move(copy);require(copy.empty());verify(moved,values,nodes,m);
            row.clear();require(row.empty());row.add(a,1,m);std::set<std::uint64_t> fresh;mark(fresh,a,m);require(row.size()==fresh.size());
        }
    }
    for(U m:{U(65),U(127),U(129),U(2049),U(INT_MAX)}) {
        reinsirk::detail::SmallFenwickRow<I> row;std::map<U,I> values;std::set<std::uint64_t> nodes;
        std::vector<U> columns={m,m/2,m/2+1};for(U c=1;c<=std::min(m,U(SMALL_POINT_LIMIT+8));++c)columns.push_back(c);
        std::sort(columns.begin(),columns.end());columns.erase(std::unique(columns.begin(),columns.end()),columns.end());
        std::mt19937 shuffle_rng(m);std::shuffle(columns.begin(),columns.end(),shuffle_rng);
        for(std::size_t j=0;j<columns.size();++j) {
            U c=columns[j];I v=j%3?I(j%13)-6:0;row.add(c,v,m);values[c]+=v;mark(nodes,c,m);verify(row,values,nodes,m);
            row.add(c,-v,m);values[c]-=v;verify(row,values,nodes,m);
            if(j>=SMALL_POINT_LIMIT-3 && j<=SMALL_POINT_LIMIT+2){auto copy=row;verify(copy,values,nodes,m);auto moved=std::move(copy);require(copy.empty());verify(moved,values,nodes,m);}
        }
    }
    // 整数型の少点保持と、浮動型・利用者型の従来経路を確認する。
    std::mt19937_64 rng(UINT64_C(98171235));
    for(bool floating:{false,true}){
        DynamicFenwickTree2D<double> bit(1000001,INT_MAX);std::map<std::pair<int,int>,long double> ref;
        long double magnitude=0;
        for(int k=0;k<700;++k){int x=k%4==0?1000000:int(rng()%1000001);int y=k%7==0?INT_MAX-1:int(rng()%INT_MAX);double v=floating?(double(int(rng()%200)-100)/10.0):double(int(rng()%101)-50);
            bit.add(x,y,v);ref[{x,y}]+=v;magnitude+=std::abs(v);
            for(int q=0;q<3;++q){int l=int(rng()%1000002),r=int(rng()%1000002),b=int(rng()%(std::uint64_t(INT_MAX)+1)),t=int(rng()%(std::uint64_t(INT_MAX)+1));if(l>r)std::swap(l,r);if(b>t)std::swap(b,t);
                long double expected=0;for(auto [xy,w]:ref)if(l<=xy.first&&xy.first<r&&b<=xy.second&&xy.second<t)expected+=w;
                require(std::abs(static_cast<long double>(bit.sum(l,b,r,t))-expected)<=1e-11L*(1+magnitude));
            }
        }
    }
    DynamicFenwickTree2D<std::uint64_t> uns(1,INT_MAX);uns.add(0,0,UINT64_MAX);uns.add(0,0,2);require(uns.get(0,0)==1);uns.add(0,INT_MAX-1,3);require(uns.sum_all()==4);
    DynamicFenwickTree2D<std::int32_t> small(1,INT_MAX);for(int c:{0,1,2,3,INT_MAX-1})small.add(0,c,7);require(small.sum_all()==35);
    std::printf("{\"row_assertions\":%ju,\"floating_tolerance\":\"1e-11*(1+sum_abs_updates)\",\"status\":\"pass\"}\n",static_cast<std::uintmax_t>(checks));
}
