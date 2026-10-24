// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_CIRCLE_TEST_HEADER
#define SIRKIT_CIRCLE_TEST_HEADER "../../../Geometry/Circle.cpp"
#endif
#include SIRKIT_CIRCLE_TEST_HEADER
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using Big = boost::multiprecision::cpp_int;
using Decimal = boost::multiprecision::cpp_dec_float_100;
using Kind = geo::CircleIntersectionKind;
std::uint64_t checks = 0;
std::string context;
void require(bool condition) {
    ++checks;
    if (!condition) { std::cerr << "circle check " << checks << " failed: " << context << "\n"; std::abort(); }
}
Decimal exact(long double value) {
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<long double>::max_digits10) << value;
    return Decimal(out.str());
}
struct IntegerCircle { std::int64_t x, y, r; };
Kind reference(IntegerCircle a, IntegerCircle b) {
    const Big dx = Big(b.x) - a.x, dy = Big(b.y) - a.y;
    const Big squared = dx * dx + dy * dy;
    const Big sum = Big(a.r) + b.r, difference = Big(a.r) - b.r;
    if (squared == 0 && a.r == b.r) return a.r == 0 ? Kind::Tangent : Kind::Coincident;
    if (squared > sum * sum || squared < difference * difference) return Kind::None;
    if (squared == sum * sum || squared == difference * difference) return Kind::Tangent;
    return Kind::TwoPoints;
}
geo::Circle convert(IntegerCircle c, long double scale = 1) {
    return {{c.x * scale, c.y * scale}, c.r * scale};
}
void residual(const geo::RealPoint& point, const geo::Circle& circle) {
    require(std::isfinite(point.x) && std::isfinite(point.y));
    const Decimal dx = exact(point.x) - exact(circle.center.x);
    const Decimal dy = exact(point.y) - exact(circle.center.y);
    const Decimal distance = sqrt(dx * dx + dy * dy);
    const long double magnitude = std::max({std::abs(circle.center.x), std::abs(circle.center.y), circle.radius});
    const long double allowance = 256 * std::numeric_limits<long double>::epsilon() * magnitude;
    require(abs(distance - exact(circle.radius)) <= exact(allowance));
}
void shape(const geo::CircleIntersection& hit, Kind expected) {
    if (hit.kind != expected) std::cerr << "kind actual=" << int(hit.kind) << " expected=" << int(expected) << "\n";
    require(hit.kind == expected);
    require(hit.points.size() == (expected == Kind::Tangent ? 1 : expected == Kind::TwoPoints ? 2 : 0));
}
void strict_case(IntegerCircle ia, IntegerCircle ib, long double scale = 1) {
    const geo::Circle a = convert(ia, scale), b = convert(ib, scale);
    const Kind expected = reference(ia, ib);
    const auto first = geo::intersection(a, b, geo::Epsilon{0, 0});
    const auto second = geo::intersection(b, a, geo::Epsilon{0, 0});
    shape(first, expected); shape(second, expected);
    require(circle_intersection_kind(a, b, geo::Epsilon{0, 0}) == expected);
    require(circle_intersection_kind(b, a, geo::Epsilon{0, 0}) == expected);
    for (const auto& p : first.points) { residual(p, a); residual(p, b); }
    for (const auto& p : second.points) { residual(p, a); residual(p, b); }
}
void exact_inputs() {
    const std::array<IntegerCircle, 11> fixed{{
        {0, 0, 0}, {0, 0, 1}, {0, 0, 5}, {6, 0, 5}, {10, 0, 5},
        {11, 0, 5}, {3, 0, 2}, {2, 0, 2}, {5, 0, 0}, {4, 3, 0}, {0, 5, 2}}};
    for (auto a : fixed) for (auto b : fixed) {
        strict_case(a, b);
        strict_case(a, b, 0.0625L);
    }
    std::mt19937_64 random(0xC1AC1E);
    for (int trial = 0; trial < 3000; ++trial) {
        auto circle = [&] {
            return IntegerCircle{std::int64_t(random() % 20001) - 10000,
                                 std::int64_t(random() % 20001) - 10000,
                                 std::int64_t(random() % 10001)};
        };
        auto a = circle(), b = trial % 29 == 0 ? a : circle();
        strict_case(a, b, trial % 2 ? 1.0L : 0.0625L);
    }
}
void known_points_and_scales() {
    std::vector<long double> scales{1e-200L, 1e-150L, 1.0L, 1e150L, 1e200L};
    scales.push_back(std::ldexp(1.0L, std::numeric_limits<long double>::max_exponent * 3 / 4));
    scales.push_back(std::ldexp(1.0L, std::numeric_limits<long double>::min_exponent * 3 / 4));
    for (long double scale : scales) {
        { std::ostringstream label; label << "scale " << scale; context = label.str(); }
        require(std::isfinite(scale) && scale > 0);
        const geo::Circle a{{0, 0}, 5 * scale}, b{{6 * scale, 0}, 5 * scale};
        const auto hit = geo::intersection(a, b, geo::Epsilon{0, 0});
        shape(hit, Kind::TwoPoints);
        require(circle_intersection_kind(a, b, geo::Epsilon{0, 0}) == Kind::TwoPoints);
        for (const auto& p : hit.points) {
            require(std::abs(p.x / scale - 3) <= 64 * std::numeric_limits<long double>::epsilon());
            require(std::abs(std::abs(p.y / scale) - 4) <= 64 * std::numeric_limits<long double>::epsilon());
        }
        context += " external";
        shape(geo::intersection(a, geo::Circle{{10 * scale, 0}, 5 * scale}, {0, 0}), Kind::Tangent);
        context += " internal";
        int exponent = 0;
        // 十進スケールでは3sと5sの個別丸めで厳密な内接から外れ得る。
        if (std::frexp(scale, &exponent) == 0.5L)
            shape(geo::intersection(a, geo::Circle{{2 * scale, 0}, 3 * scale}, {0, 0}), Kind::Tangent);
        shape(geo::intersection(a, geo::Circle{{11 * scale, 0}, 5 * scale}, {0, 0}), Kind::None);
        shape(geo::intersection(a, a, {0, 0}), Kind::Coincident);
    }
    const geo::Circle a{{1000000000, -1000000000}, 5}, b{{1000000006, -1000000000}, 5};
    const auto hit = geo::intersection(a, b, {0, 0});
    shape(hit, Kind::TwoPoints);
    for (const auto& p : hit.points) { residual(p, a); residual(p, b); }
    // 一方の向きへの射影が半径に近いだけで、ほぼ同心の2交点を接点にしない。
    const geo::Circle unit{{0, 0}, 1};
    for (long double delta : {1e-15L, 1e-18L}) {
        const auto near = geo::intersection(unit, geo::Circle{{delta, 0}, 1});
        shape(near, Kind::TwoPoints);
        require(circle_intersection_kind(unit, geo::Circle{{delta, 0}, 1}) == Kind::TwoPoints);
        for (const auto& p : near.points) require(std::abs(std::abs(p.y) - 1) < 1e-15L);
    }
}
void expected_kind(const geo::Circle& a, const geo::Circle& b, geo::Epsilon eps, Kind kind) {
    shape(geo::intersection(a, b, eps), kind);
    require(circle_intersection_kind(a, b, eps) == kind);
}
void tolerance_contract() {
    const geo::Circle a{{0, 0}, 1};
    // 許容幅の既知値。近似分類を厳密な実数の円交差とは混同しない。
    const long double tolerance = 1e-8L;
    const geo::Epsilon absolute{tolerance, 0};
    expected_kind(a, {{2 + tolerance / 4, 0}, 1}, absolute, Kind::Tangent);
    expected_kind(a, {{2 + tolerance * 4, 0}, 1}, absolute, Kind::None);
    expected_kind(a, {{2 - tolerance / 4, 0}, 1}, absolute, Kind::Tangent);
    expected_kind(a, {{2 - tolerance * 4, 0}, 1}, absolute, Kind::TwoPoints);
    const geo::Circle outer{{0, 0}, 3};
    expected_kind(outer, {{2 - tolerance / 4, 0}, 1}, absolute, Kind::Tangent);
    expected_kind(outer, {{2 - tolerance * 4, 0}, 1}, absolute, Kind::None);
    expected_kind(outer, {{2 + tolerance / 4, 0}, 1}, absolute, Kind::Tangent);
    expected_kind(outer, {{2 + tolerance * 4, 0}, 1}, absolute, Kind::TwoPoints);
    const geo::Epsilon relative{0, 1e-8L};
    const geo::Circle big{{0, 0}, 10000};
    expected_kind(big, {{20000 + 1e-5L, 0}, 10000}, relative, Kind::Tangent);
    expected_kind(big, {{20000 + 1e-2L, 0}, 10000}, relative, Kind::None);
    expected_kind(big, {{20000 - 1e-2L, 0}, 10000}, relative, Kind::TwoPoints);
    expected_kind(a, {{0, 0}, 1 + tolerance / 4}, absolute, Kind::Coincident);
    expected_kind(a, {{0, 0}, 1 + tolerance * 4}, absolute, Kind::None);
    expected_kind({{0, 0}, 0}, {{0, 0}, 0}, absolute, Kind::Tangent);
    expected_kind({{0, 0}, 0}, {{1, 0}, 1}, absolute, Kind::Tangent);
    expected_kind({{0, 0}, 0}, {{1 + tolerance / 4, 0}, 1}, absolute, Kind::Tangent);
    expected_kind({{0, 0}, 0}, {{1 + tolerance * 4, 0}, 1}, absolute, Kind::None);
}
template<class F> void domain_error(F function) {
    bool threw = false;
    try { function(); } catch (const std::domain_error&) { threw = true; }
    require(threw);
}
void invalid_input() {
    const long double inf = std::numeric_limits<long double>::infinity();
    const long double nan = std::numeric_limits<long double>::quiet_NaN();
    const std::array<geo::Circle, 6> invalid{{
        {{0, 0}, -1}, {{inf, 0}, 1}, {{0, -inf}, 1}, {{nan, 0}, 1}, {{0, 0}, inf}, {{0, 0}, nan}}};
    const geo::Circle good{{0, 0}, 1};
    for (const auto& bad : invalid) {
        domain_error([&] { geo::validate(bad); });
#ifndef SKIP_RUNTIME_CHECKS
        domain_error([&] { (void)geo::intersection(good, bad); });
        domain_error([&] { (void)geo::intersection(bad, good); });
        domain_error([&] { (void)circle_intersection_kind(good, bad); });
        domain_error([&] { (void)circle_intersection_kind(bad, good); });
#endif
    }
    geo::validate(good);
    geo::validate({{0, 0}, 0});
    require(true);
}
}
int main() {
    context = "exact_inputs"; exact_inputs();
    context = "known_points"; known_points_and_scales();
    context = "tolerance"; tolerance_contract();
    context = "invalid"; invalid_input();
    std::cout << "PASS checks=" << checks << '\n';
}
