// SPDX-License-Identifier: CC0-1.0
#ifndef HULL_HEADER
#define HULL_HEADER "../../../Geometry/Polygon.cpp"
#endif
#ifndef TEST_SORTED_HULL
#define TEST_SORTED_HULL
#endif
#include HULL_HEADER
#include <algorithm>
#include <cassert>
#include <bit>
#include <stdexcept>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <type_traits>
#include <vector>

std::uint64_t checks = 0;
// 拡張整数は独立した参照計算だけで使用する。標準代替の対象本体は拡張型を使わない。
using Wide = __int128_t;
using UWide = __uint128_t;

template<class P> Wide cross(const P& a, const P& b, const P& c) {
    Wide ux = Wide(b.x) - Wide(a.x), uy = Wide(b.y) - Wide(a.y);
    Wide vx = Wide(c.x) - Wide(a.x), vy = Wide(c.y) - Wide(a.y);
    return ux * vy - uy * vx;
}
template<class P> UWide dist2(const P& a, const P& b) {
    const Wide x = Wide(b.x) - Wide(a.x), y = Wide(b.y) - Wide(a.y);
    const UWide ax = UWide(x < 0 ? -x : x), ay = UWide(y < 0 ? -y : y);
    // 全幅64 bitの試験は薄い矩形に限定し、この和を128 bitに収める。
    return ax * ax + ay * ay;
}
// 全点に対して支持線を探す gift wrapping。対象の走査法は用いない。
template<class P> std::vector<P> reference(std::vector<P> points) {
    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end()), points.end());
    if (points.size() <= 1) return points;
    std::vector<P> result;
    std::size_t at = 0;
    do {
        result.push_back(points[at]);
        std::size_t next = at == 0 ? 1 : 0;
        for (std::size_t i = 0; i < points.size(); ++i) {
            if (i == at) continue;
            const Wide turn = cross(points[at], points[next], points[i]);
            if (turn < 0 || (turn == 0 && dist2(points[at], points[next]) < dist2(points[at], points[i]))) next = i;
        }
        at = next;
        assert(result.size() <= points.size());
    } while (at != 0);
    return result;
}

template<class P> void equal(const std::vector<P>& got, const std::vector<P>& expected, const char* tag) {
    ++checks;
    if (got != expected) {
        std::cerr << "mismatch " << tag << " check=" << checks << " got=" << got.size() << " want=" << expected.size() << '\n';
        for (const auto& p : got) std::cerr << p << ' ';
        std::cerr << '\n';
        for (const auto& p : expected) std::cerr << p << ' ';
        std::cerr << '\n';
        std::abort();
    }
}

template<class P> void check(std::vector<P> points) {
    const auto want = reference(points);
    equal(geo::convex_hull(points), want, "gift wrapping");
#ifdef TEST_SORTED_HULL
    std::sort(points.begin(), points.end());
    equal(convex_hull_sorted(points), want, "sorted gift wrapping");
#endif
}

template<class P> void float_check(std::vector<P> points, const std::vector<P>& want, geo::Epsilon eps, const char* tag) {
    equal(geo::convex_hull(points,eps), want, tag);
#ifdef TEST_SORTED_HULL
    std::sort(points.begin(),points.end());
    equal(convex_hull_sorted(points,eps), want, tag);
#endif
}

void float_tests() {
    using P = geo::Point<double>;
    const std::vector<P> line{{0,0},{1,1e-9},{2,0},{1,1e-9}};
    float_check(line, std::vector<P>{{0,0},{2,0}}, geo::Epsilon{1e-8L,0}, "absolute epsilon line");
    float_check(line, std::vector<P>{{0,0},{2,0},{1,1e-9}}, geo::Epsilon{1e-12L,0}, "absolute epsilon triangle");
    float_check(std::vector<P>{{0,0},{1e6,1e6},{2e6,2e6+1}}, std::vector<P>{{0,0},{2e6,2e6+1}}, geo::Epsilon{0,1e-6L}, "relative epsilon line");
    float_check(std::vector<P>{{0,0},{1e6,1e6},{2e6,2e6+1}}, std::vector<P>{{0,0},{1e6,1e6},{2e6,2e6+1}}, geo::Epsilon{0,1e-10L}, "relative epsilon triangle");
    std::mt19937_64 rng(0xF10A7);
    for (int rep = 0; rep < 500; ++rep) {
        std::vector<geo::PointI> ints;
        std::vector<P> doubles;
        for (int i = 0, n = int(rng()%50); i < n; ++i) {
            const auto x = std::int64_t(rng()%2001)-1000, y = std::int64_t(rng()%2001)-1000;
            ints.push_back({x,y}); doubles.push_back({double(x)/16,double(y)/16});
        }
        std::vector<P> want;
        for (const auto& p : reference(ints)) want.push_back({double(p.x)/16,double(p.y)/16});
        float_check(doubles, want, geo::Epsilon{}, "binary exact double");
    }
    using L = geo::Point<long double>;
    float_check(std::vector<L>{{0,0},{1,0},{1,1},{0,1},{0.5L,0.5L}}, std::vector<L>{{0,0},{1,0},{1,1},{0,1}}, geo::Epsilon{}, "long double square");
}

void radix_histogram_boundaries() {
    std::mt19937_64 rng(0xB17E);
    for (const auto& [sx,sy] : std::vector<std::pair<std::int64_t,std::int64_t>>{
        {255,0xFFFFFF},{0xFFFF,0xFFFFFF},{0xFFFFFF,0xFFFFFF},
        {0xFFFFFF,0xFFFFFFFF},{0x7FFFFFFF,0x7FFFFFFF}}) {
        for (const std::size_t n : {16383U,16384U,16385U}) {
            std::vector<geo::PointI> points{{0,0},{sx,0},{sx,sy},{0,sy}};
            for (std::size_t i=4;i<n;++i)
                points.push_back({std::int64_t(rng()%std::uint64_t(sx+1)),
                                  std::int64_t(rng()%std::uint64_t(sy+1))});
            std::shuffle(points.begin(),points.end(),rng);
            check(points);
        }
    }
}

void large_tests() {
    using P = geo::PointI;
    constexpr std::int64_t bound = 1000000000;
    std::mt19937_64 rng(0x64B17);
    for (std::size_t n : {2047U,2048U,2049U,4095U,4096U,4097U,16383U,16384U,16385U,65536U}) {
        std::vector<P> points{{-bound,-bound},{bound,-bound},{bound,bound},{-bound,bound}};
        for (std::size_t i = 4; i < n; ++i) {
            const auto x = std::int64_t(rng()%2000000001)-bound;
            const auto y = std::int64_t(rng()%2000000001)-bound;
            points.push_back({x,y});
        }
        std::shuffle(points.begin(),points.end(),rng);
        check(points);
        std::vector<geo::Point<double>> real;
        for (const auto& p : points) real.push_back({double(p.x),double(p.y)});
        std::vector<geo::Point<double>> want;
        for (const auto& p : reference(points)) want.push_back({double(p.x),double(p.y)});
        float_check(real,want,geo::Epsilon{0,0}, "billion integer/double");
        // 絶対値が大きくても差が小さい。標準64 bitの積・外積にも収まる。
        constexpr auto hi = std::numeric_limits<std::int64_t>::max()-bound;
        constexpr auto lo = std::numeric_limits<std::int64_t>::min()+bound;
        for (auto& p : points) p = {hi+p.x,lo+p.y};
        check(points);
        for (std::size_t i = 0; i < n; ++i) points[i] = {std::int64_t(i%31)-15,std::int64_t((i/31)%31)-15};
        std::shuffle(points.begin(),points.end(),rng);
        check(points);
        for (std::size_t i = 0; i < n; ++i) points[i] = {std::int64_t(i%31)-15,7};
        check(points);
#if defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC) && !defined(GEO_DISABLE_INT128)
        constexpr auto min = std::numeric_limits<std::int64_t>::min();
        constexpr auto max = std::numeric_limits<std::int64_t>::max();
        points = {{min,-1},{max,-1},{max,1},{min,1}};
        for (std::size_t i = 4; i < n; ++i) points.push_back({std::bit_cast<std::int64_t>(rng()),std::int64_t(rng()%3)-1});
        std::shuffle(points.begin(),points.end(),rng);
        check(points);
#endif
    }
#ifdef TEST_SORTED_HULL
#ifndef SKIP_RUNTIME_CHECKS
    for (const std::vector<P>& points : {std::vector<P>{{1,0},{0,0}},std::vector<P>{{0,0},{1,2},{1,1}},std::vector<P>{{0,0},{0,0},{0,-1}}}) {
        bool threw = false;
        try { (void)convex_hull_sorted(points); } catch (const std::invalid_argument&) { threw = true; }
        ++checks;
        if (!threw) { std::cerr << "unsorted integer input did not throw invalid_argument\n"; std::abort(); }
    }
    bool threw = false;
    try { (void)convex_hull_sorted(std::vector<geo::Point<double>>{{1,0},{0,0}},geo::Epsilon{1e-8L,0}); } catch (const std::invalid_argument&) { threw = true; }
    ++checks;
    if (!threw) { std::cerr << "unsorted double input did not throw invalid_argument\n"; std::abort(); }
#endif
#endif
}

int main() {
    radix_histogram_boundaries();
    std::mt19937_64 rng(0xB2B8C4);
    using P = geo::PointI;
    for (unsigned mask = 0; mask < 512; ++mask) {
        std::vector<P> points;
        for (unsigned i = 0; i < 9; ++i) if ((mask>>i)&1) points.push_back({int(i%3)-1,int(i/3)-1});
        check(points);
        std::reverse(points.begin(),points.end()); check(points);
        if (!points.empty()) points.insert(points.end(), points.front());
        std::shuffle(points.begin(),points.end(),rng); check(points);
    }
    for (int rep = 0; rep < 3000; ++rep) {
        std::vector<P> points;
        const int n = int(rng()%100);
        for (int i = 0; i < n; ++i) {
            const auto x = std::int64_t(rng()%101)-50, y = std::int64_t(rng()%101)-50;
            points.push_back({x,y});
            if (rep%7 == 0) points.back().y = x*3+2;
            if (rep%11 == 0) points.back().x = 0;
            if (rep%13 == 0) points.back().y = 0;
        }
        check(points);
    }
    std::vector<P> parabola;
    for (std::int64_t x = -100; x <= 100; ++x) parabola.push_back({x,x*x});
    check(parabola);
    std::reverse(parabola.begin(),parabola.end()); check(parabola);
    // 座標の絶対値ではなく差を使うこと、signed 64 bit上端付近を検査。
    const auto low = std::numeric_limits<std::int64_t>::min()+100000;
    const auto high = std::numeric_limits<std::int64_t>::max()-100000;
    check(std::vector<P>{{low,high},{low+1000,high},{low+1000,high-1000},{low,high-1000},{low+500,high-500}});
    check(std::vector<P>{{high,low},{high-1000,low},{high-1000,low+1000},{high,low+1000},{high-500,low+500}});
#if defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC) && !defined(GEO_DISABLE_INT128)
    constexpr std::int64_t a = std::int64_t{1} << 60;
    check(std::vector<P>{{-a,-a},{a,-a},{a,a},{-a,a},{0,0},{-a,0},{a,0}});
    for (int rep = 0; rep < 200; ++rep) {
        std::vector<P> points;
        for (int i = 0; i < 30; ++i) points.push_back({std::int64_t(rng()%(2*a))-a,std::int64_t(rng()%(2*a))-a});
        check(points);
    }
#endif
    // 入力型が32 bitでも差が32 bitを超えるケース。
    using Q = geo::Point<std::int32_t,std::int64_t>;
    check(std::vector<Q>{{-2000000000,0},{2000000000,0},{0,1},{0,-1}});
    float_tests();
    large_tests();
    std::cout << "passed " << checks << " independent convex hull checks\n";
}
