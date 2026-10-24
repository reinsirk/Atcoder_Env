// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_SEGMENT_TEST_HEADER
#define SIRKIT_SEGMENT_TEST_HEADER "../../../Geometry/Line_Segment.cpp"
#endif
#include SIRKIT_SEGMENT_TEST_HEADER
#include <boost/multiprecision/cpp_int.hpp>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

namespace {
using Big=boost::multiprecision::cpp_int;
std::uint64_t checks=0, outside_contract=0;
void require(bool x) {++checks;if(!x){std::cerr<<"segment check "<<checks<<" failed\n";std::abort();}}
struct V {Big x,y;};
Big cross(const V& a,const V& b){return a.x*b.y-a.y*b.x;}
template<class P>V difference(const P& a,const P& b){return{Big(a.x)-b.x,Big(a.y)-b.y};}
template<class P>bool reference(const P& a,const P& b,const P& c,const P& d){
    const auto u=difference(b,a),v=difference(d,c),w=difference(c,a);
    auto in_box=[](const P& p,const P& x,const P& y){return std::min(x.x,y.x)<=p.x&&p.x<=std::max(x.x,y.x)&&std::min(x.y,y.y)<=p.y&&p.y<=std::max(x.y,y.y);};
    if(a==b)return cross(difference(a,c),v)==0&&in_box(a,c,d);
    if(c==d)return cross(w,u)==0&&in_box(c,a,b);
    Big den=cross(u,v),t=cross(w,v),s=cross(w,u);
    if(den!=0){if(den<0){den=-den;t=-t;s=-s;}return 0<=t&&t<=den&&0<=s&&s<=den;}
    if(cross(w,u)!=0)return false;
    if(a.x!=b.x)return std::max(a.x,b.x)>=std::min(c.x,d.x)&&std::max(c.x,d.x)>=std::min(a.x,b.x);
    return std::max(a.y,b.y)>=std::min(c.y,d.y)&&std::max(c.y,d.y)>=std::min(a.y,b.y);
}
template<class P>bool valid(const P& a,const P& b,const P& c){
    const auto u=difference(b,a),v=difference(c,a);
    const Big hi=(Big{1}<<std::numeric_limits<typename P::Calc>::digits)-1,lo=-hi-1;
    for(const Big& x:std::array<Big,6>{u.x,u.y,v.x,v.y,u.x*v.y,u.y*v.x})if(x<lo||x>hi)return false;
    return true;
}
template<class T,class C>void check(geo::Segment<T,C> s,geo::Segment<T,C> t){
    if(!valid(s.a,s.b,t.a)||!valid(s.a,s.b,t.b)||!valid(t.a,t.b,s.a)||!valid(t.a,t.b,s.b)){++outside_contract;return;}
    const bool want=reference(s.a,s.b,t.a,t.b);
    require(geo::intersects(s,t)==want);require(geo::intersects(t,s)==want);
}
template<class T,class C=geo::DefaultCalc<T>>void integers(){
    using P=geo::Point<T,C>;using S=geo::Segment<T,C>;
    std::vector<P> grid;for(int x=-1;x<=1;++x)for(int y=-1;y<=1;++y)grid.push_back({T(x),T(y)});
    for(const P& a:grid)for(const P& b:grid)for(const P& c:grid)for(const P& d:grid)check(S{a,b},S{c,d});
    std::mt19937_64 rng(8291+sizeof(T));
    for(int i=0;i<10000;++i){
        const std::int64_t m=sizeof(T)==1?60:sizeof(T)==2?15000:1000000000;
        auto point=[&]{return P{T(std::int64_t(rng()%std::uint64_t(2*m+1))-m),T(std::int64_t(rng()%std::uint64_t(2*m+1))-m)};};
        S s{point(),point()},t{point(),point()};
        if(i%7==0)s.b=s.a;
        if(i%11==0)t.b=t.a;
        if(i%13==0)t.a=s.b;
        check(s,t);std::swap(s.a,s.b);check(s,t);std::swap(t.a,t.b);check(s,t);
    }
    const T lo=std::numeric_limits<T>::min(),hi=std::numeric_limits<T>::max();
    const std::vector<S> edges{{{lo,0},{hi,0}},{{0,lo},{0,hi}},{{lo,lo},{hi,hi}},
                              {{lo,hi},{hi,lo}},{{lo,0},{lo,1}},{{hi,0},{hi,1}},
                              {{0,0},{0,0}},{{lo,lo},{lo,lo}},{{hi,hi},{hi,hi}}};
    for(const auto& s:edges)for(const auto& t:edges)check(s,t);
}
template<class T>void floating(){
    using P=geo::Point<T>;using S=geo::Segment<T>;
    using I=geo::Point<std::int64_t>;
    std::mt19937_64 rng(0xF10A7);
    for(int i=0;i<10000;++i){
        std::array<I,4> a;for(auto& p:a)p={std::int64_t(rng()%2047)-1023,std::int64_t(rng()%2047)-1023};
        if(i%7==0)a[1]=a[0];
        if(i%11==0)a[3]=a[2];
        if(i%13==0)a[2]=a[1];
        auto real=[](const I& p){return P{T(p.x)/16,T(p.y)/16};};
        const S s{real(a[0]),real(a[1])},t{real(a[2]),real(a[3])};
        require(geo::intersects(s,t,geo::Epsilon{0,0})==reference(a[0],a[1],a[2],a[3]));
    }
    const S horizontal{{0,0},{4,0}};
    for(const T delta:{T(1e-10),T(1e-6)}){
        const S near{{2,delta},{2,1}};
        require(!geo::intersects(horizontal,near,geo::Epsilon{0,0}));
        require(geo::intersects(horizontal,near,geo::Epsilon{1e-8L,0})==(delta<T(1e-8)));
    }
}
}
int main(){integers<std::int8_t>();integers<std::int16_t>();integers<std::int32_t>();
    integers<std::int64_t>();integers<std::int64_t,std::int64_t>();
    floating<float>();floating<double>();floating<long double>();
    std::cout<<"PASS checks="<<checks<<" outside_contract="<<outside_contract<<'\n';}
