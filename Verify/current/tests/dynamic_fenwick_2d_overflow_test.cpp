// SPDX-License-Identifier: CC0-1.0
// 実行例: c++ -std=c++23 -O2 Verify/current/tests/dynamic_fenwick_2d_overflow_test.cpp -o /tmp/fenwick_test && /tmp/fenwick_test
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
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <type_traits>
#include FENWICK_SOURCE
#include <bit>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
using I=std::int64_t;
static std::uint64_t checks=0;
static void require(bool b){++checks;if(!b)std::abort();}
template<class T> struct CheckedFenwick {
    DynamicFenwickTree2D<T> now{1,4096};
    std::set<std::uint64_t> nodes;
    void add(int col,T v){
        now.add(0,col,v);
        for(std::uint64_t c=std::uint64_t(col)+1;c<=4096;c+=c&(0-c))nodes.insert(c);
        require(now.stored_nodes()==nodes.size());
    }
    void prefix(int r,T expected){require(now.sum(1,r)==expected);require(now.sum(0,0,1,r)==expected);}
    void rectangle(int l,int r,T expected){require(now.sum(0,l,1,r)==expected);}
    void total(T expected){require(now.sum_all()==expected);auto copy=now;require(copy.sum_all()==expected && copy.stored_nodes()==nodes.size());}
    void get(int col,T expected){require(now.get(0,col)==expected);}
};
template<class T> static void signed_tests(){
    static_assert(std::is_signed_v<T>);
    const T hi=std::numeric_limits<T>::max(),lo=std::numeric_limits<T>::min();
    {
        CheckedFenwick<T> p;p.add(0,T(-1));p.add(1,hi);p.add(1,T(1));
        // 個別点の累積がT範囲外でも、和が表現可能な範囲だけ照会する。
        p.prefix(1,T(-1));p.prefix(2,hi);p.prefix(3,hi);p.total(hi);
        for(int col=2;col<SMALL_POINT_LIMIT+4;++col){p.add(col,T(0));p.prefix(2,hi);p.total(hi);}
    }
    {
        CheckedFenwick<T> p;p.add(0,T(1));p.add(1,lo);p.add(1,T(-1));
        p.prefix(1,T(1));p.prefix(2,lo);p.total(lo);
        for(int col=2;col<SMALL_POINT_LIMIT+4;++col){p.add(col,T(0));p.prefix(2,lo);p.total(lo);}
    }
    for(int sign:{1,-1}){
        CheckedFenwick<T> p;const T a=sign>0?hi:static_cast<T>(-hi),b=static_cast<T>(-a);
        // 挿入順はA,C,B。Fenwickセルが表現可能なまま、点の再集計順を検証する。
        p.add(0,T(0));p.add(2,T(0));p.add(1,T(0));p.add(0,a);p.add(1,b);p.add(2,a);
        auto queries=[&]{p.prefix(1,a);p.prefix(2,T(0));p.prefix(3,a);p.total(a);p.get(0,a);p.get(1,b);p.get(2,a);p.rectangle(1,3,T(0));};
        queries();
        for(int col=3;col<SMALL_POINT_LIMIT+4;++col){p.add(col,T(0));queries();}
    }
}
template<class T> static void unsigned_tests(){
    static_assert(std::is_unsigned_v<T>);
    CheckedFenwick<T> p;std::map<int,T> ref;
    const T hi=std::numeric_limits<T>::max();
    auto update=[&](int col,T v){p.add(col,v);ref[col]=static_cast<T>(static_cast<std::uintmax_t>(ref[col])+static_cast<std::uintmax_t>(v));};
    auto queries=[&]{
        T total{};for(auto [col,v]:ref)total=static_cast<T>(static_cast<std::uintmax_t>(total)+static_cast<std::uintmax_t>(v));p.total(total);
        for(int r:{0,1,2,3,31,32,33,34,35,36,4096}){
            T value{};for(auto [col,v]:ref)if(col<r)value=static_cast<T>(static_cast<std::uintmax_t>(value)+static_cast<std::uintmax_t>(v));p.prefix(r,value);
        }
    };
    update(0,hi);update(0,T(1));update(0,hi);update(1,T(3));update(1,hi);queries();
    for(int col=2;col<SMALL_POINT_LIMIT+4;++col){update(col,col%3?hi:T(0));queries();}
    for(int col=0;col<SMALL_POINT_LIMIT+4;++col){update(col,T(7));queries();}
}
int main(){
    signed_tests<std::int8_t>();signed_tests<std::int16_t>();signed_tests<std::int32_t>();signed_tests<std::int64_t>();
    unsigned_tests<std::uint8_t>();unsigned_tests<std::uint16_t>();unsigned_tests<std::uint32_t>();unsigned_tests<std::uint64_t>();
    std::printf("{\"overflow_boundary_assertions\":%ju,\"integer_widths\":8,\"reference\":\"explicit_values_and_node_set\",\"status\":\"pass\"}\n",static_cast<std::uintmax_t>(checks));
}
