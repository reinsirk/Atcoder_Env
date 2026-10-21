// Verify: ローカル独立検査

// Library: Geometry/Polygon.cpp
// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef SIRKIT_GEO_EPSILON_DEFINED
#define SIRKIT_GEO_EPSILON_DEFINED
namespace geo {
// tolerance(scale): 非負有限の abs/rel から許容幅を返す。O(1)。
struct Epsilon {
    long double abs = 1e-12L;
    long double rel = 1e-12L;
    constexpr long double tolerance(long double scale) const {
        return abs + rel * std::abs(scale);
    }
};
}
#endif

#ifndef SIRKIT_GEO_POINT_DEFINED
#define SIRKIT_GEO_POINT_DEFINED
namespace geo {
#if defined(__SIZEOF_INT128__) && !defined(GEO_DISABLE_INT128) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
template<class T> using DefaultCalc = std::conditional_t<std::is_integral_v<T>, __int128_t, T>;
#else
template<class T> using DefaultCalc = std::conditional_t<std::is_integral_v<T>, std::int64_t, T>;
#endif
// Point は集成体。座標・座標演算は T、積・差・和は C に収まる有限入力を扱う。
// 比較は座標の完全一致・辞書順。+,-,*,+=,-=,*= は座標演算、点同士の * は内積、^ は外積。O(1)。
// dot/cross/norm2/dist2/orient は C、norm/dist は Real、ccw は符号を返す。dist2/orient の差は C で計算する。
// rotated90(k)/rot90(k) は反時計回り90度の k 倍のコピー/更新、arg は偏角。O(1)。
// 浮動小数点専用: /,/= は非零スカラー除算、rotated/rot はラジアン回転、normalized は非零点の単位化。
// almost_equal は成分別の誤差比較。cast<U,D>() は座標型変換。arg_cmp は零点を先頭に偏角・距離順。O(1)。
// 浮動 arg_cmp は丸めた偏角が等しければ距離順。ostream は (x,y) を出力する。O(1)。
template<class T, class C = DefaultCalc<T>>
struct Point {
    static_assert((std::is_integral_v<T> && std::is_signed_v<T>) || std::is_floating_point_v<T>);
    using Value = T;
    using Calc = C;
    using Real = std::conditional_t<std::is_floating_point_v<T>, T, long double>;
    T x = 0;
    T y = 0;
    constexpr bool operator==(const Point& p) const { return x == p.x && y == p.y; }
    constexpr bool operator!=(const Point& p) const { return !(*this == p); }
    constexpr bool operator<(const Point& p) const { return x < p.x || (x == p.x && y < p.y); }
    constexpr bool operator>(const Point& p) const { return p < *this; }
    constexpr bool operator<=(const Point& p) const { return !(p < *this); }
    constexpr bool operator>=(const Point& p) const { return !(*this < p); }
    constexpr Point operator+(const Point& p) const {
        return {T(C(x) + C(p.x)), T(C(y) + C(p.y))};
    }
    constexpr Point operator-(const Point& p) const {
        return {T(C(x) - C(p.x)), T(C(y) - C(p.y))};
    }
    constexpr Point operator-() const {
        return {T(-C(x)), T(-C(y))};
    }
    constexpr Point& operator+=(const Point& p) { return *this = *this + p; }
    constexpr Point& operator-=(const Point& p) { return *this = *this - p; }
    constexpr Point operator*(T k) const {
        return {T(C(x) * C(k)), T(C(y) * C(k))};
    }
    template<std::floating_point U> requires std::is_integral_v<T>
    Point operator*(U) const = delete;
    constexpr Point& operator*=(T k) { return *this = *this * k; }
    template<std::floating_point U> requires std::is_integral_v<T>
    Point& operator*=(U) = delete;
    friend constexpr Point operator*(T k, const Point& p) { return p * k; }
    template<std::floating_point U> requires std::is_integral_v<T>
    friend Point operator*(U, const Point&) = delete;
    Point operator/(T k) const requires std::is_floating_point_v<T> {
        if (k == 0) throw std::domain_error("zero divisor");
        return {T(C(x) / C(k)), T(C(y) / C(k))};
    }
    Point& operator/=(T k) requires std::is_floating_point_v<T> { return *this = *this / k; }
    constexpr C dot(const Point& p) const {
        return C(x) * C(p.x) + C(y) * C(p.y);
    }
    constexpr C cross(const Point& p) const {
        return C(x) * C(p.y) - C(y) * C(p.x);
    }
    constexpr C operator*(const Point& p) const { return dot(p); }
    constexpr C operator^(const Point& p) const { return cross(p); }
    constexpr C norm2() const { return dot(*this); }
    Real norm() const {
        return std::hypot(Real(x), Real(y));
    }
    constexpr C dist2(const Point& p) const {
        C dx = C(x) - C(p.x), dy = C(y) - C(p.y);
        return dx * dx + dy * dy;
    }
    Real dist(const Point& p) const {
        return std::hypot(Real(x) - Real(p.x), Real(y) - Real(p.y));
    }
    constexpr C orient(const Point& b, const Point& c) const {
        return (C(b.x) - C(x)) * (C(c.y) - C(y)) - (C(b.y) - C(y)) * (C(c.x) - C(x));
    }
    constexpr int ccw(const Point& b, const Point& c) const { C v = orient(b, c); return (v > 0) - (v < 0); }
    constexpr Point rotated90(int k = 1) const {
        k %= 4;
        if (k < 0) k += 4;
        if (k == 1) return {T(-C(y)), x};
        if (k == 2) return {T(-C(x)), T(-C(y))};
        if (k == 3) return {y, T(-C(x))};
        return *this;
    }
    constexpr void rot90(int k = 1) { *this = rotated90(k); }
    Real arg() const {
        return std::atan2(Real(y), Real(x));
    }
    Point rotated(T angle) const requires std::is_floating_point_v<T> {
        T c = std::cos(angle), s = std::sin(angle);
        return {T(C(x) * C(c) - C(y) * C(s)), T(C(x) * C(s) + C(y) * C(c))};
    }
    void rot(T angle) requires std::is_floating_point_v<T> { *this = rotated(angle); }
    Point normalized() const requires std::is_floating_point_v<T> {
#ifndef SKIP_RUNTIME_CHECKS
        if (!std::isfinite(x) || !std::isfinite(y)) throw std::domain_error("nonfinite vector");
#endif
        T m = std::max(std::abs(x), std::abs(y));
        if (m == 0) throw std::domain_error("normalize zero vector");
        T ax = x / m, ay = y / m, length = std::hypot(ax, ay);
        return {ax / length, ay / length};
    }
    bool almost_equal(const Point& p, Epsilon eps = {}) const requires std::is_floating_point_v<T> {
        return std::abs(static_cast<long double>(x) - p.x) <= eps.tolerance(std::max(std::abs(static_cast<long double>(x)), std::abs(static_cast<long double>(p.x))))
            && std::abs(static_cast<long double>(y) - p.y) <= eps.tolerance(std::max(std::abs(static_cast<long double>(y)), std::abs(static_cast<long double>(p.y))));
    }
    template<class U, class D = DefaultCalc<U>> constexpr Point<U, D> cast() const {
        return {static_cast<U>(x), static_cast<U>(y)};
    }
    static bool arg_cmp(const Point& a, const Point& b) {
        bool az = a.x == 0 && a.y == 0, bz = b.x == 0 && b.y == 0;
        if (az || bz) return az && !bz;
        bool ah = a.y < 0 || (a.y == 0 && a.x < 0), bh = b.y < 0 || (b.y == 0 && b.x < 0);
        if (ah != bh) return ah < bh;
        if constexpr (std::is_integral_v<T>) {
            C turn = C(a.x) * C(b.y) - C(a.y) * C(b.x);
            if (turn != 0) return turn > 0;
            return a.norm2() < b.norm2();
        } else {
            // 固定した偏角キーで比較し、近い角度の丸めでも推移律を保つ。
            Real aa = std::atan2(a.y, a.x), ab = std::atan2(b.y, b.x);
            if (aa < 0) aa += 2 * std::numbers::pi_v<Real>;
            if (ab < 0) ab += 2 * std::numbers::pi_v<Real>;
            if (aa != ab) return aa < ab;
            Real na = std::hypot(a.x, a.y), nb = std::hypot(b.x, b.y);
            if (na != nb) return na < nb;
            return std::max(std::abs(a.x), std::abs(a.y)) < std::max(std::abs(b.x), std::abs(b.y));
        }
    }
    friend std::ostream& operator<<(std::ostream& out, const Point& p) { return out << '(' << p.x << ',' << p.y << ')'; }
};
using PointI = Point<std::int64_t>;
using PointD = Point<double>;
using PointLD = Point<long double>;
using RealPoint = PointLD;
}
#endif

#ifndef SIRKIT_GEO_POINT_FUNCTIONS_DEFINED
#define SIRKIT_GEO_POINT_FUNCTIONS_DEFINED
namespace geo {
// 点の関数。座標は有限、整数の差・積は C に収まる入力を扱う。
// predicate_sign<T,C>(value,scale,eps): 整数は厳密、実数は許容幅で -1/0/1 を返す。O(1)。
// orientation(a,b,c,eps): 左折1、直進0、右折-1。整数では厳密。O(1)。
// to_real(p): long double 座標へ変換する。O(1)。
// distance(p,q): 2点間距離を Point::Real で返す。O(1)。

template<class T, class C> int predicate_sign(C value, C scale, Epsilon eps = {}) {
    if constexpr (std::is_integral_v<T>) return (value > 0) - (value < 0);
    else {
        long double tolerance = eps.tolerance(static_cast<long double>(scale));
        return (value > tolerance) - (value < -tolerance);
    }
}

template<class T, class C> int orientation(const Point<T, C>& a, const Point<T, C>& b, const Point<T, C>& c, Epsilon eps = {}) {
    C ux = C(b.x) - C(a.x), uy = C(b.y) - C(a.y), vx = C(c.x) - C(a.x), vy = C(c.y) - C(a.y);
    C p = ux * vy, q = uy * vx;
    if constexpr (std::is_integral_v<T>) return (p > q) - (p < q);
    else return predicate_sign<T, C>(p - q, std::abs(p) + std::abs(q), eps);
}

template<class T, class C> RealPoint to_real(const Point<T, C>& p) { return p.template cast<long double>(); }

template<class T, class C> typename Point<T, C>::Real distance(const Point<T, C>& p, const Point<T, C>& q) { return p.dist(q); }
}
#endif

#ifndef SIRKIT_GEO_LINEAR_TYPES_DEFINED
#define SIRKIT_GEO_LINEAR_TYPES_DEFINED
namespace geo {
// Segment{a,b} は閉線分（零長可）、Line{a,b} は相異なる2点で表す直線。
template<class T, class C = DefaultCalc<T>> struct Segment { Point<T, C> a, b; };
template<class T, class C = DefaultCalc<T>> struct Line { Point<T, C> a, b; };
}
#endif

#ifndef SIRKIT_GEO_ON_SEGMENT_DEFINED
#define SIRKIT_GEO_ON_SEGMENT_DEFINED
namespace geo {
// on_segment(p,s,eps): 閉線分上（端点を含む）なら true。整数では厳密。O(1)。
template<class T, class C> bool on_segment(const Point<T, C>& p, const Segment<T, C>& s, Epsilon eps = {}) {
    if (orientation(s.a, s.b, p, eps) != 0) return false;
    C x = (C(p.x) - C(s.a.x)) * (C(p.x) - C(s.b.x));
    C y = (C(p.y) - C(s.a.y)) * (C(p.y) - C(s.b.y));
    if constexpr (std::is_integral_v<T>) return x + y <= 0;
    else return predicate_sign<T, C>(x + y, std::abs(x) + std::abs(y), eps) <= 0;
}
}
#endif

#ifndef SIRKIT_GEO_LOCATION_DEFINED
#define SIRKIT_GEO_LOCATION_DEFINED
namespace geo {
// contains の戻り値: 外部・境界・内部。
enum class Location { Outside, Boundary, Inside };
}
#endif

#ifndef SIRKIT_GEO_POLYGON_DEFINED
#define SIRKIT_GEO_POLYGON_DEFINED
namespace geo {
// 多角形は vector<Point<T,C>>。先頭点の重複による閉包は不要。
// signed_area2(poly): 符号付き面積の2倍を C で返す。反時計回りが正。O(N)、空は0。
// area(poly): 面積の絶対値を long double で返す。O(N)。
// perimeter(poly): 閉じた辺の総延長。0/1点は0、2点は往復分。O(N)。
// contains(poly,p,eps): 単純多角形の内外・境界を巻き数で返す。0点は外、1/2点は境界のみ。O(N)。
// convex_hull(points,eps): 完全一致を除き、辞書順最小点から反時計回りの凸包。一直線は両端のみ。O(N log N)。
// centroid(poly,eps): 単純多角形の面積重心。3点未満または面積0なら nullopt。O(N)。
// convex_cut(poly,l,eps): 凸多角形と有向非零長直線の左側（境界含む）との共通部分。O(N)。

template<class T, class C> C signed_area2(const std::vector<Point<T, C>>& poly) {
    C result = 0;
    for (std::size_t i = 2; i < poly.size(); ++i) result += poly[0].orient(poly[i - 1], poly[i]);
    return result;
}

template<class T, class C> long double area(const std::vector<Point<T, C>>& poly) {
    return std::abs(static_cast<long double>(signed_area2(poly))) / 2;
}

template<class T, class C> long double perimeter(const std::vector<Point<T, C>>& poly) {
    if (poly.size() < 2) return 0;
    long double result = 0;
    for (std::size_t i = 0; i < poly.size(); ++i) result += poly[i].dist(poly[i + 1 == poly.size() ? 0 : i + 1]);
    return result;
}

template<class T, class C> Location contains(const std::vector<Point<T, C>>& poly, const Point<T, C>& p, Epsilon eps = {}) {
    if (poly.empty()) return Location::Outside;
    if (poly.size() == 1) return on_segment(p, Segment<T, C>{poly[0], poly[0]}, eps) ? Location::Boundary : Location::Outside;
    std::int64_t winding = 0;
    for (std::size_t i = 0; i < poly.size(); ++i) {
        const auto& a = poly[i];
        const auto& b = poly[i + 1 == poly.size() ? 0 : i + 1];
        int side = orientation(a, b, p, eps);
        if (side == 0 && on_segment(p, Segment<T, C>{a, b}, eps)) return Location::Boundary;
        if (a.y <= p.y && p.y < b.y && side > 0) ++winding;
        if (b.y <= p.y && p.y < a.y && side < 0) --winding;
    }
    return winding != 0 ? Location::Inside : Location::Outside;
}

template<class T, class C> std::vector<Point<T, C>> convex_hull(std::vector<Point<T, C>> points, Epsilon eps = {}) {
    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end()), points.end());
    if (points.size() <= 1) {
        return points;
    }
    std::vector<Point<T, C>> hull;
    auto left_turn = [&](const Point<T, C>& a, const Point<T, C>& b, const Point<T, C>& c) {
        if constexpr (std::is_integral_v<T>) return a.orient(b, c) > 0;
        else return orientation(a, b, c, eps) > 0;
    };

    if (points.size() == hull.max_size()) throw std::length_error("geometry hull capacity");
    hull.reserve(points.size() + 1);
    for (const auto& p : points) {
        while (hull.size() >= 2 && !left_turn(hull[hull.size() - 2], hull.back(), p)) hull.pop_back();
        hull.push_back(p);
    }
    std::size_t lower = hull.size();
    for (std::size_t i = points.size() - 1; i-- > 0;) {
        while (hull.size() > lower && !left_turn(hull[hull.size() - 2], hull.back(), points[i])) hull.pop_back();
        hull.push_back(points[i]);
    }
    hull.pop_back();
    return hull;
}

template<class T, class C> std::optional<RealPoint> centroid(const std::vector<Point<T, C>>& poly, Epsilon eps = {}) {
    if (poly.size() < 3) return std::nullopt;
    C total = 0;
    long double sx = 0, sy = 0;
    [[maybe_unused]] C scale = 0;
    for (std::size_t i = 2; i < poly.size(); ++i) {
        C weight = poly[0].orient(poly[i - 1], poly[i]);
        total += weight;
        if constexpr (!std::is_integral_v<T>) scale += std::abs(weight);
        long double ux = static_cast<long double>(C(poly[i - 1].x) - C(poly[0].x)), uy = static_cast<long double>(C(poly[i - 1].y) - C(poly[0].y));
        long double vx = static_cast<long double>(C(poly[i].x) - C(poly[0].x)), vy = static_cast<long double>(C(poly[i].y) - C(poly[0].y));
        sx += (ux + vx) * static_cast<long double>(weight);
        sy += (uy + vy) * static_cast<long double>(weight);
    }
    if (predicate_sign<T, C>(total, scale, eps) == 0) return std::nullopt;
    long double denominator = 3 * static_cast<long double>(total);
    return RealPoint{static_cast<long double>(poly[0].x) + sx / denominator, static_cast<long double>(poly[0].y) + sy / denominator};
}

template<class T, class C> std::vector<RealPoint> convex_cut(const std::vector<Point<T, C>>& poly, const Line<T, C>& line, Epsilon eps = {}) {
    if (line.a == line.b) throw std::domain_error("degenerate line");
    std::vector<RealPoint> result;
    result.reserve(poly.size());
    if (poly.empty()) return result;
    for (std::size_t i = 0; i < poly.size(); ++i) {
        const auto& a = poly[i];
        const auto& b = poly[i + 1 == poly.size() ? 0 : i + 1];
        int sa = orientation(line.a, line.b, a, eps), sb = orientation(line.a, line.b, b, eps);
        auto append = [&](const RealPoint& p) { if (result.empty() || result.back() != p) result.push_back(p); };
        if (sa >= 0) append(to_real(a));
        if (sa * sb < 0) {
            C da = line.a.orient(line.b, a), db = line.a.orient(line.b, b);
            long double u = static_cast<long double>(da) / (static_cast<long double>(da) - static_cast<long double>(db));
            append({static_cast<long double>(a.x) + u * static_cast<long double>(C(b.x) - C(a.x)), static_cast<long double>(a.y) + u * static_cast<long double>(C(b.y) - C(a.y))});
        }
    }
    if (result.size() > 1 && result.front() == result.back()) result.pop_back();
    return result;
}
}
#endif


// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

int main() {
    const geo::PointI a{0, 0}, b{4, 0}, c{0, 3};
    assert(a.dist2(b) == 16 && a.orient(b, c) == 12);
    assert((b.rotated90() == geo::PointI{0, 4}));
    assert(geo::orientation(a, b, c) == 1);
    const geo::PointLD p{3, 4};
    assert(std::abs(p.normalized().norm() - 1) < 1e-12L);
#ifdef SIRKIT_GEO_LINEAR_DEFINED
    const geo::Segment<std::int64_t> first{{0, 0}, {4, 0}}, overlap{{2, 0}, {6, 0}}, singleton{{1, 0}, {1, 0}};
    assert(geo::intersection(first, overlap).kind == geo::SegmentIntersectionKind::Overlap);
    assert(geo::intersection(first, singleton).kind == geo::SegmentIntersectionKind::Point);
    assert(geo::intersection(geo::Line<std::int64_t>{{0, 0}, {1, 0}}, geo::Line<std::int64_t>{{0, 1}, {1, 1}}).kind == geo::LineIntersectionKind::None);
    assert(std::abs(geo::distance(geo::PointI{1, 2}, singleton) - 2) < 1e-12L);
#endif
#ifdef SIRKIT_GEO_POLYGON_DEFINED
    const auto hull = geo::convex_hull(std::vector<geo::PointI>{a, b, c, {1, 1}, a});
    assert(hull.size() == 3 && geo::area(hull) == 6);
    assert(geo::contains(hull, geo::PointI{1, 1}) == geo::Location::Inside);
    assert(geo::contains(hull, a) == geo::Location::Boundary);
    auto center = geo::centroid(hull);
    assert(center && std::abs(center->x - 4.0L / 3) < 1e-12L && std::abs(center->y - 1) < 1e-12L);
    assert(!geo::centroid(std::vector<geo::PointI>{a, b}));
    assert(geo::convex_hull(std::vector<geo::PointI>{{0, 0}, {1, 0}, {2, 0}}).size() == 2);
#endif
#ifdef SIRKIT_GEO_CIRCLE_DEFINED
    const auto circle = geo::circumcircle(a, b, c);
    assert(circle && std::abs(circle->radius - 2.5L) < 1e-12L);
    assert(!geo::circumcircle(a, a, b));
    const geo::Circle unit{{0, 0}, 1};
    assert(geo::intersection(unit, geo::Circle{{3, 0}, 1}).kind == geo::CircleIntersectionKind::None);
    assert(geo::intersection(unit, unit).kind == geo::CircleIntersectionKind::Coincident);
    assert(geo::intersection(unit, geo::Circle{{2, 0}, 1}).kind == geo::CircleIntersectionKind::Tangent);
    assert(geo::intersection(unit, geo::Segment<long double>{{-2, 0}, {2, 0}}).size() == 2);
    assert(geo::intersection(unit, geo::Segment<long double>{{2, 0}, {3, 0}}).empty());
#endif
    std::cout << "OK\n";
}
