// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_ORIENTATION_TEST_HEADER
#define SIRKIT_ORIENTATION_TEST_HEADER "../../../Geometry/Point.cpp"
#endif
#include SIRKIT_ORIENTATION_TEST_HEADER
#include <boost/multiprecision/cpp_int.hpp>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

namespace {
using Big = boost::multiprecision::cpp_int;
std::uint64_t checks = 0, outside_contract = 0;
void require(bool value) {
    ++checks;
    if (!value) { std::cerr << "orientation check " << checks << " failed\n"; std::abort(); }
}
int sign(const Big& x) { return (x > 0) - (x < 0); }

template<class P> void check(const P& a, const P& b, const P& c) {
    const Big ux = Big(b.x) - a.x, uy = Big(b.y) - a.y;
    const Big vx = Big(c.x) - a.x, vy = Big(c.y) - a.y;
    const Big p = ux * vy, q = uy * vx;
    const Big high = (Big{1} << std::numeric_limits<typename P::Calc>::digits) - 1;
    const Big low = -high - 1;
    for (const Big& x : std::array<Big, 6>{ux,uy,vx,vy,p,q}) {
        if (x < low || x > high) { ++outside_contract; return; }
    }
    const Big determinant = p - q;
    require(geo::orientation(a,b,c) == sign(determinant));
    if (low <= determinant && determinant <= high) {
        require(Big(a.orient(b,c)) == determinant);
        require(a.ccw(b,c) == sign(determinant));
    }
}

template<class T, class C = geo::DefaultCalc<T>> void integers() {
    using P = geo::Point<T,C>;
    std::mt19937_64 rng(0xA891 + sizeof(T));
    for (int i=0;i<10000;++i) {
        const std::int64_t bound = sizeof(T)==1 ? 60 : sizeof(T)==2 ? 16000 : 1000000000;
        auto point = [&] { return P{T(std::int64_t(rng()%std::uint64_t(2*bound+1))-bound),
                                   T(std::int64_t(rng()%std::uint64_t(2*bound+1))-bound)}; };
        const P a=point(),b=point(),c=i%7==0 ? a : point();
        check(a,b,c); check(a,c,b); check(b,c,a);
    }
    const auto lo=std::numeric_limits<T>::min(), hi=std::numeric_limits<T>::max();
    const std::vector<P> edges{{lo,0},{T(lo+7),T(-5)},{T(lo+11),13},
                               {hi,0},{T(hi-7),T(-5)},{T(hi-11),13},
                               {0,lo},{0,hi},{lo,lo},{hi,hi},{0,0}};
    for (const P& a:edges) for (const P& b:edges) for (const P& c:edges) check(a,b,c);
}

template<class T> void floating() {
    using P=geo::Point<T>;
    std::mt19937_64 rng(0xF10A);
    // 座標・許容幅を二進有理数に限定し、参照側は整数だけで閾値を比較する。
    for (int i=0;i<10000;++i) {
        std::array<std::int64_t,6> v;
        for(auto& x:v) x=std::int64_t(rng()%2047)-1023;
        if(i%11==0) {v[4]=v[0];v[5]=v[1];}
        const P a{T(v[0])/16,T(v[1])/16},b{T(v[2])/16,T(v[3])/16},c{T(v[4])/16,T(v[5])/16};
        const Big p=Big(v[2]-v[0])*(v[5]-v[1]),q=Big(v[3]-v[1])*(v[4]-v[0]);
        for (const int absolute:{0,64,1024}) for (const int relative:{0,1,32}) {
            const Big value=(p-q)*1024, tolerance=Big(absolute)*256+Big(relative)*(abs(p)+abs(q));
            const int expected=int(value>tolerance)-int(value < -tolerance);
            require(geo::orientation(a,b,c,geo::Epsilon{absolute/1024.0L,relative/1024.0L})==expected);
        }
    }
}
}

int main() {
    integers<std::int8_t>(); integers<std::int16_t>();
    integers<std::int32_t>(); integers<std::int64_t>();
    integers<std::int64_t,std::int64_t>();
    using P=geo::Point<std::int64_t,std::int64_t>;
    check(P{0,0},P{3000000000,-3000000000},P{3000000000,3000000000});
    floating<float>(); floating<double>(); floating<long double>();
    std::cout << "PASS checks=" << checks << " outside_contract=" << outside_contract << '\n';
}
