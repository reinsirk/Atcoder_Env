// SPDX-License-Identifier: CC0-1.0
// 実行例: c++ -std=c++23 -O2 Verify/current/tests/dynamic_fenwick_2d_test.cpp -o /tmp/fenwick_test && /tmp/fenwick_test
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
#include <stdexcept>
#include <type_traits>
#include <utility>
#include FENWICK_SOURCE
#include FENWICK_SOURCE
#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <random>
#include <set>
#include <tuple>
#include <vector>

using I = std::int64_t;
static std::uint64_t checks = 0, scenarios = 0;
static void require(bool ok) { ++checks; if (!ok) { std::fprintf(stderr,"failure after %ju checks, %ju scenarios\n",static_cast<std::uintmax_t>(checks),static_cast<std::uintmax_t>(scenarios)); std::abort(); } }
// 加減算群としてのみ利用できる利用側の型。暗黙変換・比較・乗算なし。
struct Value {
    I a = 0, b = 0;
    Value() = default;
    explicit Value(I x) : a(x), b(x * 3) {}
    Value& operator+=(const Value& r) { a += r.a; b += r.b; return *this; }
    Value& operator-=(const Value& r) { a -= r.a; b -= r.b; return *this; }
    friend Value operator-(Value l, const Value& r) { return l -= r; }
};
struct ThrowingSwapValue {
    I a=0;
    ThrowingSwapValue() = default;
    explicit ThrowingSwapValue(I x) noexcept : a(x) {}
    ThrowingSwapValue& operator+=(const ThrowingSwapValue& r) { a+=r.a; return *this; }
    ThrowingSwapValue& operator-=(const ThrowingSwapValue& r) { a-=r.a; return *this; }
    friend ThrowingSwapValue operator-(ThrowingSwapValue l,const ThrowingSwapValue& r) { return l-=r; }
    friend void swap(ThrowingSwapValue& a,ThrowingSwapValue& b) noexcept(false) { std::swap(a.a,b.a); }
};
static_assert(std::is_nothrow_default_constructible_v<Value> && std::is_nothrow_copy_assignable_v<Value> && std::is_nothrow_swappable_v<Value>);
static_assert(!std::is_nothrow_swappable_v<ThrowingSwapValue>);
static I unpack(const ThrowingSwapValue& v) { return v.a; }
static I unpack(I v) { return v; }
static I unpack(const Value& v) { require(v.b == v.a * 3); return v.a; }
template<class T> static T make_value(I v) { return T(v); }
static std::mt19937_64 rng(UINT64_C(0x91760ade5410));
using Points = std::map<std::pair<int,int>, I>;
using Nodes = std::set<std::pair<std::uint64_t,std::uint64_t>>;
static I reference(const Points& p, int x0,int y0,int x1,int y1) {
    I result = 0;
    for (const auto& [xy,v] : p) if (x0 <= xy.first && xy.first < x1 && y0 <= xy.second && xy.second < y1) result += v;
    return result;
}
static void mark(Nodes& nodes,int n,int m,int x,int y) {
    for (std::uint64_t i = std::uint64_t(x)+1; i <= std::uint64_t(n); i += i & (0-i))
        for (std::uint64_t j = std::uint64_t(y)+1; j <= std::uint64_t(m); j += j & (0-j)) nodes.emplace(i,j);
}
static int endpoint(int n) {
    switch (rng()%8) { case 0: return 0; case 1: return n; case 2: return n/2; default: return int(rng()%(std::uint64_t(n)+1)); }
}
static int position(int n) {
    switch (rng()%8) { case 0: return 0; case 1: return n-1; case 2: return n/2; default: return int(rng()%std::uint64_t(n)); }
}
template<class T> static void reads(const DynamicFenwickTree2D<T>& bit,const Points& points,int n,int m,int count) {
    auto nodes = bit.stored_nodes();
    for (int z=0;z<count;++z) {
        int l=endpoint(n), r=endpoint(n), b=endpoint(m), t=endpoint(m);
        if(l>r) std::swap(l,r);
        if(b>t) std::swap(b,t);
        require(unpack(bit.sum(l,b,r,t)) == reference(points,l,b,r,t));
        require(unpack(bit.sum(r,t)) == reference(points,0,0,r,t));
        if(n && m) { int x=position(n),y=position(m); auto it=points.find({x,y}); require(unpack(bit.get(x,y)) == (it==points.end()?0:it->second)); }
        require(unpack(bit.sum_all()) == reference(points,0,0,n,m));
        require(bit.stored_nodes() == nodes);
    }
}
template<class T> static void lifecycle(DynamicFenwickTree2D<T>& bit,const Points& points,int n,int m) {
    auto copy = bit; reads(copy,points,n,m,10);
    if(n && m) { copy.add(0,0,make_value<T>(17)); require(unpack(bit.sum_all()) == reference(points,0,0,n,m)); }
    copy = bit; reads(copy,points,n,m,4); auto* same = &copy; copy = *same; reads(copy,points,n,m,2);
    auto moved = std::move(copy);
    require(copy.empty() && copy.rows()==0 && copy.cols()==0 && copy.stored_nodes()==0 && unpack(copy.sum_all())==0);
    reads(moved,points,n,m,6);
    copy.reset(3,5); copy.add(2,4,make_value<T>(99)); copy = std::move(moved);
    require(moved.empty() && moved.rows()==0 && moved.cols()==0 && moved.stored_nodes()==0 && unpack(moved.sum_all())==0);
    reads(copy,points,n,m,4); copy = std::move(*same); reads(copy,points,n,m,2);
    DynamicFenwickTree2D<T> other(2,2); other.add(1,1,make_value<T>(7)); copy.swap(other);
    require(copy.rows()==2 && copy.cols()==2 && unpack(copy.get(1,1))==7); reads(other,points,n,m,3);
    bit.clear(); require(bit.rows()==n && bit.cols()==m && bit.stored_nodes()==0 && unpack(bit.sum_all())==0);
    reads(bit,{},n,m,3);
    if(n&&m) { bit.add(n-1,m-1,make_value<T>(11)); require(unpack(bit.get(n-1,m-1))==11); }
    for(auto [a,b]:std::vector<std::pair<int,int>>{{0,5},{5,0},{65,64},{64,65},{3,2049},{INT_MAX,65},{INT_MAX,1},{7,7}}) {
        bit.reset(a,b); require(bit.rows()==a && bit.cols()==b && bit.stored_nodes()==0 && unpack(bit.sum_all())==0);
        if(a&&b) { bit.add(a-1,b-1,make_value<T>(5)); require(unpack(bit.sum_all())==5); require(unpack(bit.get(a-1,b-1))==5); }
    }
}
template<class T> static void sparse_case(int n,int m,int operations,bool ownership=true) {
    ++scenarios;
    DynamicFenwickTree2D<T> bit(n,m); Points points; Nodes nodes;
    require(bit.rows()==n && bit.cols()==m && bit.empty()==(!n||!m) && bit.stored_nodes()==0);
    reads(bit,points,n,m,3); bit.reserve(17);
    for(int k=0;k<operations;++k) {
        if(n&&m) {
            int x=position(n), y=position(m); I value=I(rng()%101)-50;
            switch(k%9) {
                case 0: value=0; [[fallthrough]];
                case 1: case 2: case 3: bit.add(x,y,make_value<T>(value)); points[{x,y}] += value; break;
                default: bit.set(x,y,make_value<T>(value)); points[{x,y}] = value; break;
            }
            mark(nodes,n,m,x,y); require(bit.stored_nodes()==nodes.size());
            require(unpack(bit.get(x,y))==points[{x,y}]);
            if(k%17==0) { bit.add(x,y,make_value<T>(-points[{x,y}])); points[{x,y}]=0; require(bit.stored_nodes()==nodes.size()); }
        }
        reads(bit,points,n,m,2);
    }
    if(ownership) lifecycle(bit,points,n,m);
}
template<class T> static void dense_naive() {
    for(int n : {0,1,2,3,7,8,15,16,31,32,63,64,65}) for(int m : {0,1,2,3,7,8,31,32,63,64,65}) {
        ++scenarios; DynamicFenwickTree2D<T> bit(n,m); std::vector<I> cells(std::size_t(n)*m);
        for(int step=0;step<90;++step) {
            if(n&&m) { int x=position(n),y=position(m); I v=I(rng()%19)-9;
                if(step%3) { bit.add(x,y,make_value<T>(v)); cells[std::size_t(x)*m+y]+=v; }
                else { bit.set(x,y,make_value<T>(v)); cells[std::size_t(x)*m+y]=v; }
            }
            int l=endpoint(n),r=endpoint(n),b=endpoint(m),t=endpoint(m);
            if(l>r) std::swap(l,r);
            if(b>t) std::swap(b,t);
            I expected=0; for(int x=l;x<r;++x)for(int y=b;y<t;++y)expected+=cells[std::size_t(x)*m+y];
            require(unpack(bit.sum(l,b,r,t))==expected);
        }
    }
}
template<class T> static void promotion_case(int n,int m,int columns) {
    ++scenarios;
    DynamicFenwickTree2D<T> bit(n,m); Points points; Nodes nodes;
    for(int j=0;j<columns;++j) {
        const int x=n-1; const I delta=j%5==0?0:I(j%17)-8;
        bit.add(x,j,make_value<T>(delta)); points[{x,j}]+=delta; mark(nodes,n,m,x,j);
        require(bit.stored_nodes()==nodes.size());
        if(j%31==0) reads(bit,points,n,m,2);
    }
    reads(bit,points,n,m,40);
    for(int j=0;j<columns;j+=7) {
        bit.set(n-1,j,make_value<T>(j%13-6)); points[{n-1,j}]=j%13-6;
        require(bit.stored_nodes()==nodes.size()); require(unpack(bit.get(n-1,j))==points[{n-1,j}]);
    }
    reads(bit,points,n,m,30);
    // コピー後のclear/resetで、寸法・保持形式を変えて再利用する。
    auto copy=bit;
    reads(copy,points,n,m,4);
    for(int pass=0;pass<4;++pass) {
        copy.clear(); require(copy.stored_nodes()==0 && unpack(copy.sum_all())==0);
        int width=pass%2?4096:65;
        int rows=pass%2?1:1000001;
        copy.reset(rows,width); Points next; Nodes next_nodes;
        int count=width==65?65:1600;
        for(int j=0;j<count;++j) { I v=j%11-5; copy.add(rows-1,j,make_value<T>(v)); next[{rows-1,j}]=v; mark(next_nodes,rows,width,rows-1,j); }
        require(copy.stored_nodes()==next_nodes.size()); reads(copy,next,rows,width,6);
    }
    lifecycle(bit,points,n,m);
}
template<class T> static void few_points_case(int n,int m) {
    ++scenarios; DynamicFenwickTree2D<T> bit(n,m); Points points; Nodes nodes;
    // 同列反復・零加算・取消しと、保持点数の閾値を超える移行を確認する。
    std::vector<int> cols={0,m-1,m/2,1,m/2-1,2,m-2,3,15,16,17,31,32,33,7,8,9,4,5,6};
    for(int c=4;c<std::min(m,SMALL_POINT_LIMIT+12);++c) cols.push_back(c);
    std::sort(cols.begin(),cols.end()); cols.erase(std::unique(cols.begin(),cols.end()),cols.end());
    if(cols.size()>3) std::rotate(cols.begin()+1,cols.end()-1,cols.end());
    const std::vector<int> row_choices={n-1,n/2,0};
    for(int row:row_choices) {
        for(std::size_t phase=0;phase<cols.size();++phase) {
            int col=cols[phase];
            for(I delta:{I(0),I(7),I(-7),I(19),I(0),I(-3)}) {
                bit.add(row,col,make_value<T>(delta)); points[{row,col}]+=delta;mark(nodes,n,m,row,col);
                require(bit.stored_nodes()==nodes.size()); require(unpack(bit.get(row,col))==points[{row,col}]);
                reads(bit,points,n,m,3);
                for(auto [l,r]:std::vector<std::pair<int,int>>{{col,col},{col,col+1},{0,col},{0,col+1},{col,m},{0,m}}) {
                    require(unpack(bit.sum(row,l,row+1,r))==reference(points,row,l,row+1,r));
                }
            }
        }
    }
    auto copy=bit;reads(copy,points,n,m,6);
    for(int cycle=0;cycle<3;++cycle) {
        copy.clear();require(copy.stored_nodes()==0 && unpack(copy.sum_all())==0);
        Points next; Nodes seen;
        for(int index: {0,1,2,3,0,4,1,5}) {int col=cols[std::size_t(index)%cols.size()]; I delta=index%2?0:13;
            copy.add(n-1,col,make_value<T>(delta));next[{n-1,col}]+=delta;mark(seen,n,m,n-1,col);
            require(copy.stored_nodes()==seen.size());reads(copy,next,n,m,3);
        }
    }
}
#ifndef SKIP_RUNTIME_CHECKS
template<class E,class F> static void throws(F f) { bool thrown=false; try { f(); } catch(const E&) { thrown=true; } require(thrown); }
static void invalid_inputs() {
    throws<std::invalid_argument>([]{DynamicFenwickTree2D<I> b(-1,1);});
    throws<std::invalid_argument>([]{DynamicFenwickTree2D<I> b(1,-1);});
    DynamicFenwickTree2D<I> b(3,5); b.add(1,2,7);
    throws<std::out_of_range>([&]{b.add(-1,2,1);}); throws<std::out_of_range>([&]{b.add(3,2,1);});
    throws<std::out_of_range>([&]{b.add(2,-1,1);}); throws<std::out_of_range>([&]{b.add(2,5,1);});
    throws<std::out_of_range>([&]{(void)b.get(-1,2);}); throws<std::out_of_range>([&]{(void)b.get(1,5);});
    throws<std::out_of_range>([&]{b.set(3,0,2);}); throws<std::out_of_range>([&]{(void)b.sum(-1,0);});
    throws<std::out_of_range>([&]{(void)b.sum(0,6);}); throws<std::out_of_range>([&]{(void)b.sum(0,0,4,4);});
    throws<std::invalid_argument>([&]{(void)b.sum(2,0,1,4);}); throws<std::invalid_argument>([&]{(void)b.sum(0,3,2,2);});
    throws<std::invalid_argument>([&]{b.reset(-1,5);}); require(b.rows()==3 && b.cols()==5 && b.sum_all()==7);
    throws<std::invalid_argument>([&]{b.reset(2,-1);}); require(b.rows()==3 && b.cols()==5 && b.sum_all()==7);
}
#endif
int main() {
    dense_naive<I>(); dense_naive<Value>();
    for(auto [n,m]:std::vector<std::pair<int,int>>{{1,2049},{1,INT_MAX},{1000001,65},{1000001,2049},{1000001,INT_MAX},{INT_MAX,INT_MAX}}) {
        few_points_case<I>(n,m);few_points_case<Value>(n,m);few_points_case<ThrowingSwapValue>(n,m);
    }
    const std::vector<std::pair<int,int>> dims = {
        {0,INT_MAX},{INT_MAX,0},{1,1},{1,64},{1,65},{65,1},{64,64},{65,65},
        {17,2047},{17,2048},{17,2049},{2048,2048},{2049,2048},
        {4096,1024},{4097,1024},{4097,2049},{1000001,1},{1000001,64},{1000001,65},
        {INT_MAX,1},{INT_MAX,64},{INT_MAX,65},{INT_MAX,2048},{INT_MAX,INT_MAX}};
    for(auto [n,m]:dims) { sparse_case<I>(n,m,96); sparse_case<Value>(n,m,40); }
    sparse_case<I>(1000000,1,8,false);
    for(auto [n,m,count]:std::vector<std::tuple<int,int,int>>{{1,4096,1600},{65537,65,65},{1000001,65,65}}) {
        promotion_case<I>(n,m,count); promotion_case<Value>(n,m,count); promotion_case<ThrowingSwapValue>(n,m,count);
    }
    // 非常に広い列でも、疎な更新・最終列・零セルの計数を維持する。
    sparse_case<I>(1,2097153,80);
    sparse_case<Value>(1,2097153,40);
#ifndef SKIP_RUNTIME_CHECKS
    invalid_inputs();
#endif
    std::printf("{\"scenarios\":%ju,\"assertions\":%ju,\"status\":\"pass\"}\n",static_cast<std::uintmax_t>(scenarios),static_cast<std::uintmax_t>(checks));
}
