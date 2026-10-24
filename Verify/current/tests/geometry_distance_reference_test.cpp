// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_DISTANCE_TEST_HEADER
#define SIRKIT_DISTANCE_TEST_HEADER "../../../Geometry/Line_Segment.cpp"
#endif
#include SIRKIT_DISTANCE_TEST_HEADER
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>

namespace {
using Big = boost::multiprecision::cpp_int;
using Decimal = boost::multiprecision::cpp_dec_float_100;
std::uint64_t checks = 0;
std::string context;
void require(bool condition) {
    ++checks;
    if (!condition) { std::cerr << "distance check " << checks << " failed: " << context << '\n'; std::abort(); }
}
Decimal binary_value(long double value) {
    if (value == 0) return 0;
    int exponent = 0;
    long double fraction = std::frexp(std::abs(value), &exponent);
    Big significand = 0;
    for (int i = 0; i < std::numeric_limits<long double>::digits; ++i) {
        fraction *= 2;
        significand <<= 1;
        if (fraction >= 1) { ++significand; fraction -= 1; }
    }
    Decimal result(significand.convert_to<std::string>());
    result *= pow(Decimal(2), exponent - std::numeric_limits<long double>::digits);
    return value < 0 ? -result : result;
}
struct DPoint { Decimal x, y; };
struct Reference { Decimal distance; Decimal span; };
Reference decimal_reference(DPoint p, DPoint a, DPoint b) {
    const Decimal dx = b.x - a.x, dy = b.y - a.y;
    const Decimal ux = p.x - a.x, uy = p.y - a.y;
    const Decimal vx = p.x - b.x, vy = p.y - b.y;
    const Decimal den = dx * dx + dy * dy, dot = ux * dx + uy * dy;
    const Decimal span = std::max({abs(dx), abs(dy), abs(ux), abs(uy), abs(vx), abs(vy)});
    if (den == 0 || dot <= 0) return {sqrt(ux * ux + uy * uy), span};
    if (dot >= den) return {sqrt(vx * vx + vy * vy), span};
    return {abs(ux * dy - uy * dx) / sqrt(den), span};
}
Reference integer_reference(geo::PointI p, geo::PointI a, geo::PointI b) {
    const Big dx = Big(b.x) - a.x, dy = Big(b.y) - a.y;
    const Big ux = Big(p.x) - a.x, uy = Big(p.y) - a.y;
    const Big vx = Big(p.x) - b.x, vy = Big(p.y) - b.y;
    const Big den = dx * dx + dy * dy, dot = ux * dx + uy * dy;
    const auto decimal = [](const Big& n) { return Decimal(n.convert_to<std::string>()); };
    const Big span = std::max({abs(dx), abs(dy), abs(ux), abs(uy), abs(vx), abs(vy)});
    if (den == 0 || dot <= 0) return {sqrt(decimal(ux * ux + uy * uy)), decimal(span)};
    if (dot >= den) return {sqrt(decimal(vx * vx + vy * vy)), decimal(span)};
    return {decimal(abs(ux * dy - uy * dx)) / sqrt(decimal(den)), decimal(span)};
}
void distance_result(long double actual, const Reference& expected) {
    require(std::isfinite(actual) && actual >= 0);
    // 単位化・差・内外積の丸めを、入力の相対スケールで評価する。
    // 大座標の小距離へ一律の絶対精度を要求する試験ではない。
    const Decimal epsilon(std::numeric_limits<long double>::epsilon());
    const Decimal allowance = 128 * epsilon * expected.span + 32 * epsilon * expected.distance;
    const Decimal error = abs(binary_value(actual) - expected.distance);
    if (error > allowance) {
        std::cerr << "actual=" << actual << " expected=" << expected.distance << " error=" << error
                  << " allowance=" << allowance << '\n';
    }
    require(error <= allowance);
}
void integer_case(geo::PointI p, geo::PointI a, geo::PointI b) {
    const auto representable = [](geo::PointI q) {
        return Big(static_cast<long double>(q.x)) == Big(q.x) && Big(static_cast<long double>(q.y)) == Big(q.y);
    };
    const auto converted = [](geo::PointI q) {
        return DPoint{binary_value(static_cast<long double>(q.x)), binary_value(static_cast<long double>(q.y))};
    };
    // long doubleの仮数が短いABIでは、API通り座標変換の丸めを先に反映する。
    const auto expected = representable(p) && representable(a) && representable(b)
        ? integer_reference(p, a, b) : decimal_reference(converted(p), converted(a), converted(b));
    distance_result(geo::distance(p, geo::Segment<std::int64_t>{a, b}), expected);
    distance_result(geo::distance(p, geo::Segment<std::int64_t>{b, a}), expected);
    const auto swap = [](geo::PointI q) { return geo::PointI{q.y, q.x}; };
    distance_result(geo::distance(swap(p), geo::Segment<std::int64_t>{swap(a), swap(b)}), expected);
}
void integers() {
    using P = geo::PointI;
    const std::array<P, 9> grid{{{-1,-1},{-1,0},{-1,1},{0,-1},{0,0},{0,1},{1,-1},{1,0},{1,1}}};
    for (auto p : grid) for (auto a : grid) for (auto b : grid) integer_case(p, a, b);
    std::mt19937_64 random(0xD157A);
    for (int trial = 0; trial < 1800; ++trial) {
        const auto point = [&] { return P{std::int64_t(random() % 2000000001) - 1000000000,
                                            std::int64_t(random() % 2000000001) - 1000000000}; };
        P p = point(), a = point(), b = trial % 19 == 0 ? a : point();
        integer_case(p, a, b);
        for (P* q : {&p,&a,&b}) { q->x += 1000000000; q->y -= 1000000000; }
        integer_case(p, a, b);
    }
    const auto low = std::numeric_limits<std::int64_t>::lowest(), high = std::numeric_limits<std::int64_t>::max();
    for (const auto& v : std::array<std::array<P, 3>, 8>{{
        {{{0,1},{low,0},{high,0}}}, {{{0,0},{low,low},{high,high}}},
        {{{low,high},{low,low},{high,low}}}, {{{high,high},{high,high},{high,high}}},
        {{{high-1,high},{high-2,high-2},{high,high-2}}},
        {{{low+1,low+2},{low,low},{low+2,low}}},
        {{{1000000000,1000000001},{0,0},{1000000001,1000000002}}},
        {{{1000000000,1000000000},{0,0},{1000000001,1000000001}}}
    }}) integer_case(v[0], v[1], v[2]);
    // 軸平行の既知値は、巨大座標でも小距離を直接確認する。
    require(geo::distance(P{0,1}, geo::Segment<std::int64_t>{{low,0},{high,0}}) == 1);
    if constexpr (std::numeric_limits<long double>::digits >= 64)
        require(geo::distance(P{high-1,high}, geo::Segment<std::int64_t>{{high-2,high-2},{high,high-2}}) == 2);
}
template<class T> void floating() {
    using P = geo::Point<T>;
    std::mt19937_64 random(0xF10A7 + sizeof(T));
    for (int trial = 0; trial < 600; ++trial) {
        const auto point = [&] { return P{T(int(random() % 2049) - 1024) / T(16),
                                            T(int(random() % 2049) - 1024) / T(16)}; };
        const P p = point(), a = point(), b = trial % 17 == 0 ? a : point();
        const auto exact = [](P q) { return DPoint{binary_value(q.x), binary_value(q.y)}; };
        const auto expected = decimal_reference(exact(p), exact(a), exact(b));
        distance_result(geo::distance(p, geo::Segment<T>{a,b}), expected);
        distance_result(geo::distance(p, geo::Segment<T>{b,a}), expected);
    }
}
void scales() {
    std::array<long double, 8> scales{{1e-200L,1e-150L,1.0L,1e150L,1e200L,
        std::ldexp(1.0L, std::numeric_limits<long double>::min_exponent * 3 / 4),
        std::ldexp(1.0L, std::numeric_limits<long double>::max_exponent * 3 / 4),
        std::numeric_limits<long double>::min() * 16}};
    for (long double scale : scales) {
        context = "scale " + std::to_string(std::ilogb(scale));
        const geo::RealPoint a{0,0}, b{3*scale,4*scale};
        for (auto p : {geo::RealPoint{-4*scale,3*scale}, geo::RealPoint{-3*scale,-4*scale},
                       geo::RealPoint{6*scale,8*scale}, geo::RealPoint{0,0}}) {
            const auto exact = [](geo::RealPoint q) { return DPoint{binary_value(q.x),binary_value(q.y)}; };
            const auto expected = decimal_reference(exact(p),exact(a),exact(b));
            distance_result(geo::distance(p,geo::Segment<long double>{a,b}),expected);
            distance_result(geo::distance(p,geo::Segment<long double>{b,a}),expected);
        }
    }
    const long double large = std::numeric_limits<long double>::max() / 4;
    require(geo::distance(geo::RealPoint{0,1},geo::Segment<long double>{{-large,0},{large,0}}) == 1);
    require(geo::distance(geo::RealPoint{1,0},geo::Segment<long double>{{0,-large},{0,large}}) == 1);
}
template<class T> void narrow_integers() {
    using P = geo::Point<T>;
    require(geo::distance(P{2,3},geo::Segment<T>{{0,0},{4,0}}) == 3);
    require(geo::distance(P{-3,4},geo::Segment<T>{{0,0},{4,0}}) == 5);
    require(geo::distance(P{7,4},geo::Segment<T>{{0,0},{4,0}}) == 5);
}
void integer_related() {
    using S = geo::Segment<std::int64_t>;
    require(geo::distance(S{{0,0},{4,0}},S{{0,3},{4,3}},{0,0}) == 3);
    require(geo::distance(S{{0,0},{4,0}},S{{2,-1},{2,1}},{0,0}) == 0);
    using P = geo::Point<std::int64_t,double>;
    require(geo::distance(P{2,3},geo::Segment<std::int64_t,double>{{0,0},{4,0}}) == 3);
}
void related_apis() {
    using P = geo::RealPoint;
    const geo::Segment<long double> segment{{0,0},{4,0}};
    for (P p : {P{-3,4},P{2,3},P{7,4},P{0,0}}) {
        const auto nearest = geo::closest_point(p,segment);
        require(nearest.y == 0 && nearest.x >= 0 && nearest.x <= 4);
        require(std::abs(geo::distance(p,segment)-p.dist(nearest)) < 1e-17L);
        require(geo::distance(p,geo::Line<long double>{{0,0},{4,0}}) == std::abs(p.y));
    }
    require((geo::closest_point(P{2,3},geo::Segment<long double>{{1,1},{1,1}}) == P{1,1}));
    require(geo::distance(segment,geo::Segment<long double>{{0,3},{4,3}},{0,0}) == 3);
    require(geo::distance(segment,geo::Segment<long double>{{2,-1},{2,1}},{0,0}) == 0);
    for (bool projection : {false,true}) {
        bool threw = false;
        try {
            if (projection) (void)geo::projection(P{2,3},geo::Line<long double>{{1,1},{1,1}});
            else (void)geo::distance(P{2,3},geo::Line<long double>{{1,1},{1,1}});
        } catch (const std::domain_error&) { threw = true; }
        require(threw);
    }
}
}
int main() {
    narrow_integers<std::int8_t>(); narrow_integers<std::int16_t>(); narrow_integers<std::int32_t>(); integer_related();
    require(std::numeric_limits<long double>::radix == 2);
    context="integers"; integers();
    context="float"; floating<float>();
    context="double"; floating<double>();
    context="long double"; floating<long double>();
    context="scales"; scales();
    context="related"; related_apis();
    std::cout << "PASS checks=" << checks << '\n';
}
