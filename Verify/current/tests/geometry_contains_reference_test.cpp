// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_CONTAINS_TEST_HEADER
#define SIRKIT_CONTAINS_TEST_HEADER "../../../Geometry/Polygon.cpp"
#endif
#include SIRKIT_CONTAINS_TEST_HEADER
#include <boost/multiprecision/cpp_int.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

namespace {
using Big = boost::multiprecision::cpp_int;
using IPoint = geo::Point<std::int64_t>;
using Location = geo::Location;
std::uint64_t checks = 0, outside_contract = 0;

void require(bool value) {
    ++checks;
    if (!value) {
        std::cerr << "contains check " << checks << " failed\n";
        std::abort();
    }
}

// 本体の巻き数とは別に、水平半直線と辺の有理数交点の偶奇を数える。
template<class P>
Location reference(const std::vector<P>& poly, const P& p) {
    bool inside = false;
    for (std::size_t i = 0; i < poly.size(); ++i) {
        const auto& a = poly[i];
        const auto& b = poly[(i + 1) % poly.size()];
        const Big dx = Big(b.x) - a.x, dy = Big(b.y) - a.y;
        const Big px = Big(p.x) - a.x, py = Big(p.y) - a.y;
        if (dx * py == dy * px &&
            std::min(a.x, b.x) <= p.x && p.x <= std::max(a.x, b.x) &&
            std::min(a.y, b.y) <= p.y && p.y <= std::max(a.y, b.y)) return Location::Boundary;
        if ((a.y > p.y) == (b.y > p.y)) continue;
        // 交点のx = (a.x * dy + (p.y - a.y) * dx) / dy。
        Big numerator = Big(a.x) * dy + py * dx;
        Big denominator = dy;
        if (denominator < 0) { numerator = -numerator; denominator = -denominator; }
        if (numerator > Big(p.x) * denominator) inside = !inside;
    }
    return inside ? Location::Inside : Location::Outside;
}

// 範囲外の計算は呼び出さない。保守的に全辺の差・積・差分を検査する。
template<class P>
bool valid(const std::vector<P>& poly, const P& p) {
    using C = typename P::Calc;
    const Big hi = (Big{1} << std::numeric_limits<C>::digits) - 1, lo = -hi - 1;
    auto fits = [&](const Big& x) { return lo <= x && x <= hi; };
    for (std::size_t i = 0; i < poly.size(); ++i) {
        const auto& a = poly[i];
        const auto& b = poly[(i + 1) % poly.size()];
        const Big dx = Big(b.x) - a.x, dy = Big(b.y) - a.y;
        const Big px = Big(p.x) - a.x, py = Big(p.y) - a.y;
        for (const Big& x : std::array<Big, 7>{dx, dy, px, py, dx * py, dy * px, dx * py - dy * px})
            if (!fits(x)) return false;
        if (poly.size() == 1 && (!fits(px * px) || !fits(py * py) || !fits(px * px + py * py))) return false;
    }
    return true;
}

template<class P>
void check(const std::vector<P>& poly, const P& p) {
    if (!valid(poly, p)) { ++outside_contract; return; }
    const auto want = reference(poly, p);
    const auto got = geo::contains(poly, p);
    if (got != want) {
        std::cerr << "query " << p << " polygon";
        for (const auto& a : poly) std::cerr << ' ' << a;
        std::cerr << " want=" << int(want) << " got=" << int(got) << '\n';
    }
    require(got == want);
}

std::vector<std::vector<IPoint>> shapes() {
    return {{}, {{2, 2}}, {{0, 0}, {8, 0}}, {{0, 0}, {0, 8}}, {{0, 0}, {8, 8}},
        {{2, 2}, {2, 2}}, {{0, 0}, {8, 0}, {8, 8}, {0, 8}},
        {{0, 4}, {4, 0}, {8, 4}, {4, 8}}, {{0, 0}, {8, 2}, {4, 8}},
        {{0, 0}, {8, 0}, {8, 2}, {2, 2}, {2, 8}, {0, 8}},
        {{0, 0}, {8, 0}, {8, 8}, {6, 8}, {6, 2}, {2, 2}, {2, 8}, {0, 8}},
        {{0, 0}, {4, 0}, {8, 0}, {8, 4}, {8, 8}, {4, 8}, {0, 8}, {0, 4}},
        {{0, 0}, {8, 0}, {8, 8}, {6, 4}, {4, 8}, {2, 4}, {0, 8}}};
}

template<class P>
std::vector<P> convert(const std::vector<IPoint>& poly, std::int64_t tx = 0, std::int64_t ty = 0) {
    std::vector<P> result;
    for (const auto& a : poly) result.push_back({static_cast<typename P::Value>(a.x + tx), static_cast<typename P::Value>(a.y + ty)});
    return result;
}

template<class T, class C = geo::DefaultCalc<T>>
void integers() {
    using P = geo::Point<T, C>;
    for (auto original : shapes()) {
        for (int variant = 0; variant < 4; ++variant) {
            auto poly = original;
            if (variant & 1) std::reverse(poly.begin(), poly.end());
            if (!poly.empty() && variant & 2) {
                std::rotate(poly.begin(), poly.begin() + poly.size() / 2, poly.end());
                poly.insert(poly.begin() + 1, poly.front());
                poly.push_back(poly.front());
            }
            const std::int64_t tx = variant == 3 ? -30 : 0, ty = variant == 3 ? 19 : 0;
            const auto actual = convert<P>(poly, tx, ty);
            for (int x = -2; x <= 10; ++x) for (int y = -2; y <= 10; ++y)
                check(actual, P{T(x + tx), T(y + ty)});
        }
    }
    // 既存の広い総合試験に加え、各頂点と辺の中点を必ず照会する。
    std::mt19937_64 rng(0xC07A175);
    for (int trial = 0; trial < 60; ++trial) {
        std::vector<IPoint> original;
        const int n = 2 + int(rng() % 9);
        for (int x = 0; x < n; ++x) original.push_back({2 * x, 2 + std::int64_t(rng() % 6) * 2});
        for (int x = n; x-- > 0;) original.push_back({2 * x, -2 - std::int64_t(rng() % 6) * 2});
        for (int order = 0; order < 2; ++order) {
            const auto poly = convert<P>(original);
            for (std::size_t i = 0; i < original.size(); ++i) {
                const auto& a = original[i];
                const auto& b = original[(i + 1) % original.size()];
                check(poly, P{T(a.x), T(a.y)});
                check(poly, P{T((a.x + b.x) / 2), T((a.y + b.y) / 2)});
            }
            for (int k = 0; k < 12; ++k)
                check(poly, P{T(std::int64_t(rng() % 25) - 3), T(std::int64_t(rng() % 31) - 15)});
            std::reverse(original.begin(), original.end());
        }
    }
    const T lo = std::numeric_limits<T>::min(), hi = std::numeric_limits<T>::max();
    const std::vector<std::vector<P>> extremes{
        {{lo, lo}, {T(lo + 16), lo}, {T(lo + 16), T(lo + 16)}, {lo, T(lo + 16)}},
        {{T(hi - 16), T(hi - 16)}, {hi, T(hi - 16)}, {hi, hi}, {T(hi - 16), hi}},
        {{lo, 0}, {hi, 0}, {hi, 1}, {lo, 1}},
        {{lo, lo}, {hi, lo}, {hi, hi}, {lo, hi}}, {{lo, lo}}, {{hi, hi}}};
    const std::array<P, 12> queries{{{lo, lo}, {hi, hi}, {0, 0}, {0, 1}, {0, 2},
        {T(lo + 8), T(lo + 8)}, {T(lo + 16), T(lo + 16)}, {T(lo + 17), T(lo + 8)},
        {T(hi - 8), T(hi - 8)}, {T(hi - 16), T(hi - 16)}, {T(hi - 17), T(hi - 8)}, {lo, hi}}};
    for (auto poly : extremes) for (int order = 0; order < 2; ++order) {
        for (const auto& p : queries) check(poly, p);
        std::reverse(poly.begin(), poly.end());
    }
}

template<class T>
void floating() {
    using P = geo::Point<T>;
    // 小さな整数の1/16倍なので、積・差も各型で厳密に表現できる。
    for (auto original : shapes()) for (int order = 0; order < 2; ++order) {
        std::vector<P> poly;
        for (const auto& a : original) poly.push_back({T(a.x) / 16, T(a.y) / 16});
        for (int x = -3; x <= 11; ++x) for (int y = -3; y <= 11; ++y)
            require(geo::contains(poly, P{T(x) / 16, T(y) / 16}, geo::Epsilon{0, 0}) == reference(original, IPoint{x, y}));
        std::reverse(original.begin(), original.end());
    }
    const std::vector<P> square{{0, 0}, {4, 0}, {4, 4}, {0, 4}};
    for (T delta : {T(1e-10L), T(1e-6L)}) {
        const bool near = delta < T(1e-8L);
        require(geo::contains(square, P{2, -delta}, geo::Epsilon{0, 0}) == Location::Outside);
        require(geo::contains(square, P{2, delta}, geo::Epsilon{0, 0}) == Location::Inside);
        require(geo::contains(square, P{2, -delta}, geo::Epsilon{1e-8L, 0}) == (near ? Location::Boundary : Location::Outside));
        require(geo::contains(square, P{2, delta}, geo::Epsilon{1e-8L, 0}) == (near ? Location::Boundary : Location::Inside));
        require(geo::contains(square, P{-delta, 2}, geo::Epsilon{1e-8L, 0}) == (near ? Location::Boundary : Location::Outside));
    }
    const std::vector<P> point{{0, 0}};
    require(geo::contains(point, P{T(1e-5L), 0}, geo::Epsilon{1e-8L, 0}) == Location::Boundary);
    require(geo::contains(point, P{T(1e-3L), 0}, geo::Epsilon{1e-8L, 0}) == Location::Outside);
    const std::vector<P> diagonal{{0, 0}, {1024, 1024}, {1024, 0}};
    const P above{512, T(512) + T(1) / 1024};
    require(geo::contains(diagonal, above, geo::Epsilon{0, 0}) == Location::Outside);
    require(geo::contains(diagonal, above, geo::Epsilon{0, 1e-5L}) == Location::Boundary);
    require(geo::contains(diagonal, above, geo::Epsilon{0, 1e-8L}) == Location::Outside);
}

// Cへの変換丸めを含む既存の結果を固定する。厳密幾何の参照とは分ける。
template<class C>
void mixed_calculation(std::int64_t base) {
    using P = geo::Point<std::int64_t, C>;
    std::vector<P> triangle{{base, base}, {base + 1, base + 256}, {base + 1024, base + 1024}};
    const std::array<P, 6> queries{{triangle[0], triangle[1], triangle[2],
        {base, base + 32}, {base + 500, base + 600}, {base, base + 1000}}};
    const std::array<Location, 6> expected{{Location::Boundary, Location::Boundary, Location::Boundary,
        Location::Boundary, Location::Inside, Location::Outside}};
    for (int order = 0; order < 2; ++order) {
        for (std::size_t i = 0; i < queries.size(); ++i) require(geo::contains(triangle, queries[i]) == expected[i]);
        std::reverse(triangle.begin(), triangle.end());
    }
}

// 4頂点単位の走査・切替閾値・末尾の余りに境界が重なる単純多角形。
template<class C = geo::DefaultCalc<std::int64_t>>
void large_boundaries() {
    using P = geo::Point<std::int64_t, C>;
    constexpr std::array<int, 16> sizes{127, 128, 129, 130, 131, 255, 256, 257,
        258, 259, 511, 512, 513, 514, 515, 516};
    for (int n : sizes) for (int pattern = 0; pattern < 3; ++pattern) {
        std::vector<P> original;
        const int upper = n - 2;
        for (int i = 0; i < upper; ++i) {
            const bool alternating = pattern == 1 || (pattern == 2 && upper / 3 <= i && i < 2 * upper / 3);
            original.push_back({2 * i, alternating && (i & 1) ? -8 : 8});
        }
        original.push_back({2 * (upper - 1), -64});
        original.push_back({0, -64});
        for (int variant = 0; variant < 3; ++variant) {
            auto poly = original;
            if (variant == 1) std::reverse(poly.begin(), poly.end());
            if (variant == 2) std::rotate(poly.begin(), poly.begin() + 1, poly.end());
            // 先頭、ブロック境界、途中、末尾、閉じ辺の端点と中点。
            for (std::size_t i : std::array<std::size_t, 10>{0, 1, 3, 4, 7, 8,
                    std::size_t(n / 2), std::size_t(n - 3), std::size_t(n - 2), std::size_t(n - 1)}) {
                const auto& a = poly[i];
                const auto& b = poly[(i + 1) % poly.size()];
                check(poly, a);
                check(poly, P{(a.x + b.x) / 2, (a.y + b.y) / 2});
            }
            // 全体の上下分離、外側、straddle連続と上下分離への復帰。
            const std::int64_t mid = upper - 1;
            for (P p : std::array<P, 8>{{{mid, 128}, {mid, -128}, {-2, 0},
                    {2 * upper, 0}, {mid, 0}, {mid, 1}, {mid, -32}, {mid, -63}}}) check(poly, p);
        }
    }
}
}

int main() {
    integers<std::int8_t>(); integers<std::int16_t>(); integers<std::int32_t>();
    integers<std::int64_t>(); integers<std::int64_t, std::int64_t>();
    integers<std::int16_t, std::int32_t>();
    floating<float>(); floating<double>(); floating<long double>();
    mixed_calculation<float>(1000000000);
    mixed_calculation<double>(std::int64_t{1} << 53);
    large_boundaries<>();
    large_boundaries<std::int64_t>();
    std::cout << "PASS checks=" << checks << " outside_contract=" << outside_contract << '\n';
}
