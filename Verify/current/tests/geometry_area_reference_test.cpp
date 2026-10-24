// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_AREA_TEST_HEADER
#define SIRKIT_AREA_TEST_HEADER "../../../Geometry/Polygon.cpp"
#endif
#include SIRKIT_AREA_TEST_HEADER
#include <boost/multiprecision/cpp_int.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
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
    if (!value) { std::cerr << "area check " << checks << " failed\n"; std::abort(); }
}
struct ExactPoint { Big x, y; };
Big shoelace(const std::vector<ExactPoint>& polygon) {
    Big sum = 0;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const auto& a = polygon[i];
        const auto& b = polygon[(i + 1) % polygon.size()];
        sum += a.x * b.y - a.y * b.x;
    }
    return sum;
}
template<class C> bool fits(const Big& value) {
    const Big high = (Big{1} << std::numeric_limits<C>::digits) - 1;
    return -high - 1 <= value && value <= high;
}
// 本体を呼ぶ前に、三角扇の全中間値が計算型に収まることを多倍長で確認する。
template<class C> bool valid(const std::vector<ExactPoint>& polygon) {
    Big sum = 0;
    for (std::size_t i = 2; i < polygon.size(); ++i) {
        const Big x = polygon[i - 1].x - polygon[0].x;
        const Big y = polygon[i - 1].y - polygon[0].y;
        const Big u = polygon[i].x - polygon[0].x;
        const Big v = polygon[i].y - polygon[0].y;
        const Big p = x * v, q = y * u, turn = p - q;
        sum += turn;
        for (const Big& value : std::array<Big, 8>{x, y, u, v, p, q, turn, sum})
            if (!fits<C>(value)) return false;
    }
    return true;
}
void area_result(long double actual, const Big& twice, long double denominator = 1) {
    const long double expected = abs(twice).convert_to<long double>() / (2 * denominator);
    require(std::isfinite(actual));
    require(std::abs(actual - expected) <= 2 * std::numeric_limits<long double>::epsilon() * expected);
    if (twice == 0) require(actual == 0);
}
template<class T, class C> void integer_polygon(const std::vector<geo::Point<T, C>>& polygon) {
    std::vector<ExactPoint> exact;
    for (const auto& p : polygon) exact.push_back({Big(p.x), Big(p.y)});
    if (!valid<C>(exact)) { ++outside_contract; return; }
    const Big expected = shoelace(exact);
    require(Big(geo::signed_area2(polygon)) == expected);
    area_result(geo::area(polygon), expected);
    if (polygon.size() == 3) {
        require(Big(signed_triangle_area2(polygon[0], polygon[1], polygon[2])) == expected);
        area_result(triangle_area(polygon[0], polygon[1], polygon[2]), expected);
    }
}
template<class T, class C> void integer_variants(std::vector<geo::Point<T, C>> polygon, bool translate) {
    integer_polygon(polygon);
    std::reverse(polygon.begin(), polygon.end());
    integer_polygon(polygon);
    if (!polygon.empty()) std::rotate(polygon.begin(), polygon.begin() + 1, polygon.end());
    integer_polygon(polygon);
    if (translate) {
        // 呼出元は座標型の端を避けた入力だけを渡す。
        for (auto& p : polygon) { p.x = T(p.x + T(2)); p.y = T(p.y - T(3)); }
        integer_polygon(polygon);
    }
}
template<class T, class C = geo::DefaultCalc<T>> void integers() {
    using P = geo::Point<T, C>;
    const std::vector<std::vector<P>> fixed{
        {}, {{0, 0}}, {{1, 2}, {-3, 4}},
        {{0, 0}, {4, 0}, {4, 4}, {2, 1}, {0, 4}}, // 凹多角形
        {{0, 0}, {4, 0}, {4, 4}, {0, 4}, {0, 0}}, // 閉包点の重複
        {{0, 0}, {0, 0}, {1, 1}, {2, 2}, {2, 2}}};
    for (const auto& polygon : fixed) integer_variants(polygon, true);
    std::vector<P> grid;
    for (int x = -1; x <= 1; ++x) for (int y = -1; y <= 1; ++y) grid.push_back({T(x), T(y)});
    for (const P& a : grid) for (const P& b : grid) for (const P& c : grid)
        integer_polygon(std::vector<P>{a, b, c});
    std::mt19937_64 random(0xA2EA + sizeof(T) + std::numeric_limits<C>::digits);
    for (int trial = 0; trial < 1000; ++trial) {
        const int bound = sizeof(T) == 1 ? 50 : sizeof(T) == 2 ? 10000 : 100000000;
        const std::size_t size = trial % 3 == 0 ? 3 : std::size_t(random() % 34);
        std::vector<P> polygon;
        for (std::size_t i = 0; i < size; ++i) {
            auto coordinate = [&] { return T(std::int64_t(random() % std::uint64_t(2 * bound + 1)) - bound); };
            polygon.push_back({coordinate(), coordinate()});
        }
        // 任意頂点列の符号付き面積和も靴紐公式と照合する。
        integer_variants(polygon, true);
    }
    const T low = std::numeric_limits<T>::lowest(), high = std::numeric_limits<T>::max();
    const std::vector<std::vector<P>> boundaries{
        {{low, low}, {T(low + T(1)), low}, {low, T(low + T(1))}},
        {{high, high}, {T(high - T(1)), high}, {high, T(high - T(1))}},
        {{low, 0}, {high, 0}, {low, 1}},
        {{0, low}, {0, high}, {1, low}},
        {{low, low}, {high, low}, {high, high}, {low, high}},
        {{low, low}, {high, high}, {low, low}},
        {{low, 0}, {high, 0}, {high, 1}, {low, 1}}};
    for (const auto& polygon : boundaries) integer_variants(polygon, false);
}
template<class T> void dyadic() {
    using P = geo::Point<T>;
    std::mt19937_64 random(0xD1AD1C);
    for (int trial = 0; trial < 1500; ++trial) {
        const std::size_t size = trial % 3 == 0 ? 3 : std::size_t(random() % 34);
        std::vector<ExactPoint> exact;
        std::vector<P> polygon;
        for (std::size_t i = 0; i < size; ++i) {
            const int x = int(random() % 255) - 127, y = int(random() % 255) - 127;
            exact.push_back({x, y}); polygon.push_back({T(x) / 16, T(y) / 16});
        }
        const Big twice = shoelace(exact);
        const T expected = twice.convert_to<T>() / T(256);
        // 分子の全中間値を24ビット未満に収め、floatでも厳密一致させる。
        require(geo::signed_area2(polygon) == expected);
        area_result(geo::area(polygon), twice, 256);
        if (size == 3) {
            require(signed_triangle_area2(polygon[0], polygon[1], polygon[2]) == expected);
            area_result(triangle_area(polygon[0], polygon[1], polygon[2]), twice, 256);
        }
        std::reverse(polygon.begin(), polygon.end());
        require(geo::signed_area2(polygon) == -expected);
    }
}
template<class C> void floating_calculation(std::int64_t base) {
    using P = geo::Point<std::int64_t, C>;
    std::mt19937_64 random(0xCA1C);
    for (int trial = 0; trial < 1000; ++trial) {
        const std::size_t size = trial % 3 == 0 ? 3 : std::size_t(random() % 34);
        std::vector<P> polygon;
        std::vector<ExactPoint> rounded;
        for (std::size_t i = 0; i < size; ++i) {
            const P p{base + std::int64_t(random() % 256), base + std::int64_t(random() % 256)};
            polygon.push_back(p);
            // 計算型への座標変換による丸めはAPI通りに反映し、面積は独立に計算する。
            rounded.push_back({std::int64_t(C(p.x)), std::int64_t(C(p.y))});
        }
        const Big twice = shoelace(rounded);
        require(geo::signed_area2(polygon) == twice.convert_to<C>());
        area_result(geo::area(polygon), twice);
        if (size == 3) {
            require(signed_triangle_area2(polygon[0], polygon[1], polygon[2]) == twice.convert_to<C>());
            area_result(triangle_area(polygon[0], polygon[1], polygon[2]), twice);
        }
    }
}
}
int main() {
    integers<std::int8_t>(); integers<std::int16_t>(); integers<std::int32_t>();
    integers<std::int64_t>(); integers<std::int64_t, std::int64_t>();
    dyadic<float>(); dyadic<double>(); dyadic<long double>();
    floating_calculation<float>(std::int64_t{1} << 30);
    floating_calculation<double>(std::int64_t{1} << 53);
    floating_calculation<long double>(std::int64_t{1} << 53);
    std::cout << "PASS checks=" << checks << " outside_contract=" << outside_contract << '\n';
}
