#define SKIP_RUNTIME_CHECKS
// Verify: https://atcoder.jp/contests/math-and-algorithm/tasks/math_and_algorithm_ah

// Library: Geometry/Line_Segment.cpp
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

#ifndef REINSIRK_GEO_EPSILON_DEFINED
#define REINSIRK_GEO_EPSILON_DEFINED
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

#ifndef REINSIRK_GEO_POINT_DEFINED
#define REINSIRK_GEO_POINT_DEFINED
namespace geo {
#if defined(__SIZEOF_INT128__) && !defined(GEO_DISABLE_INT128) && !defined(REINSIRK_FORCE_STANDARD_ARITHMETIC)
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

#ifndef REINSIRK_GEO_POINT_FUNCTIONS_DEFINED
#define REINSIRK_GEO_POINT_FUNCTIONS_DEFINED
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

#ifndef REINSIRK_GEO_LINEAR_TYPES_DEFINED
#define REINSIRK_GEO_LINEAR_TYPES_DEFINED
namespace geo {
// Segment{a,b} は閉線分（零長可）、Line{a,b} は相異なる2点で表す直線。
template<class T, class C = DefaultCalc<T>> struct Segment { Point<T, C> a, b; };
template<class T, class C = DefaultCalc<T>> struct Line { Point<T, C> a, b; };
}
#endif

#ifndef REINSIRK_GEO_ON_SEGMENT_DEFINED
#define REINSIRK_GEO_ON_SEGMENT_DEFINED
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

#ifndef REINSIRK_GEO_SEGMENT_PREDICATES_DEFINED
#define REINSIRK_GEO_SEGMENT_PREDICATES_DEFINED
namespace geo {
// intersects(s,t,eps): 閉線分同士が端点・重なりを含め交わるか。O(1)。
template<class T, class C> bool intersects(const Segment<T, C>& s, const Segment<T, C>& t, Epsilon eps = {}) {
    int a = orientation(s.a, s.b, t.a, eps), b = orientation(s.a, s.b, t.b, eps);
    if constexpr (std::is_integral_v<T>) {
        if (a * b > 0) return false;
    }
    int c = orientation(t.a, t.b, s.a, eps), d = orientation(t.a, t.b, s.b, eps);
    if (a == 0 && on_segment(t.a, s, eps)) return true;
    if (b == 0 && on_segment(t.b, s, eps)) return true;
    if (c == 0 && on_segment(s.a, t, eps)) return true;
    if (d == 0 && on_segment(s.b, t, eps)) return true;
    return a * b < 0 && c * d < 0;
}
}
#endif

#ifndef REINSIRK_GEO_LINEAR_RESULTS_DEFINED
#define REINSIRK_GEO_LINEAR_RESULTS_DEFINED
namespace geo {
// 直線・線分の交差種別。交点は RealPoint、重なりは辞書順の両端点。
enum class LineIntersectionKind { None, Point, Coincident };
enum class SegmentIntersectionKind { None, Point, Overlap };
struct LineIntersection { LineIntersectionKind kind = LineIntersectionKind::None; std::optional<RealPoint> point; };
struct SegmentIntersection { SegmentIntersectionKind kind = SegmentIntersectionKind::None; std::vector<RealPoint> points; };
}
#endif

#ifndef REINSIRK_GEO_LINEAR_DEFINED
#define REINSIRK_GEO_LINEAR_DEFINED
namespace geo {
// 点・直線・閉線分の関数。交点・射影・距離は long double で近似する。
// projection(p,l): long double 変換後も非零長の直線 l 上への射影。O(1)。
// reflection(p,l): 直線 l に関して対称な点。projection と同じ前提。O(1)。
// closest_point(p,s): 閉線分上の最近点。long double 変換後の零長線分は a を返す。O(1)。
// distance(p,l): 点と非零長直線の距離。O(1)。
// distance(p,s): 点と閉線分の距離（零長可）。O(1)。
// distance(s,t,eps): 閉線分間距離。intersects が真なら0。O(1)。
// intersection(l,m,eps): 非零長直線の交差種別と近似交点。O(1)。
// intersection(s,t,eps): 閉線分の交点、または元の座標で辞書順に選んだ重なりの両端点。O(1)。

template<class T, class C> RealPoint projection(const Point<T, C>& p, const Line<T, C>& l) {
    RealPoint a = to_real(l.a), b = to_real(l.b), q = to_real(p);
    RealPoint d = b - a;
    if (d.x == 0 && d.y == 0) throw std::domain_error("degenerate line");
    RealPoint u = d.normalized();
    return a + u * ((q - a).dot(u));
}

template<class T, class C> RealPoint reflection(const Point<T, C>& p, const Line<T, C>& l) {
    RealPoint q = projection(p, l);
    return q + (q - to_real(p));
}

template<class T, class C> RealPoint closest_point(const Point<T, C>& p, const Segment<T, C>& s) {
    RealPoint a = to_real(s.a), b = to_real(s.b), q = to_real(p), d = b - a;
    if (d.x == 0 && d.y == 0) return a;
    RealPoint u = d.normalized();
    long double along = (q - a).dot(u);
    if (along <= 0) return a;
    if ((q - b).dot(u) >= 0) return b;
    return a + u * along;
}

template<class T, class C> long double distance(const Point<T, C>& p, const Line<T, C>& l) {
    RealPoint a = to_real(l.a), b = to_real(l.b), q = to_real(p), d = b - a;
    if (d.x == 0 && d.y == 0) throw std::domain_error("degenerate line");
    return std::abs((q - a).cross(d.normalized()));
}

template<class T, class C> long double distance(const Point<T, C>& p, const Segment<T, C>& s) {
    return to_real(p).dist(closest_point(p, s));
}

template<class T, class C> long double distance(const Segment<T, C>& s, const Segment<T, C>& t, Epsilon eps = {}) {
    if (intersects(s, t, eps)) return 0;
    return std::min({distance(s.a, t), distance(s.b, t), distance(t.a, s), distance(t.b, s)});
}

template<class T, class C> LineIntersection intersection(const Line<T, C>& l, const Line<T, C>& m, Epsilon eps = {}) {
    if (l.a == l.b || m.a == m.b) throw std::domain_error("degenerate line");
    C ux = C(l.b.x) - C(l.a.x), uy = C(l.b.y) - C(l.a.y);
    C vx = C(m.b.x) - C(m.a.x), vy = C(m.b.y) - C(m.a.y);
    C p = ux * vy, q = uy * vx, determinant = p - q;
    int sign;
    if constexpr (std::is_integral_v<T>) sign = (determinant > 0) - (determinant < 0);
    else sign = predicate_sign<T, C>(determinant, std::abs(p) + std::abs(q), eps);
    if (sign == 0) {
        if (orientation(l.a, l.b, m.a, eps) == 0) return {LineIntersectionKind::Coincident, std::nullopt};
        return {};
    }
    C wx = C(m.a.x) - C(l.a.x), wy = C(m.a.y) - C(l.a.y);
    long double t = static_cast<long double>(wx * vy - wy * vx) / static_cast<long double>(determinant);
    return {LineIntersectionKind::Point, RealPoint{static_cast<long double>(l.a.x) + t * static_cast<long double>(ux), static_cast<long double>(l.a.y) + t * static_cast<long double>(uy)}};
}

template<class T, class C> SegmentIntersection intersection(const Segment<T, C>& s, const Segment<T, C>& t, Epsilon eps = {}) {
    std::array<Point<T, C>, 4> candidates;
    std::size_t n = 0;
    if (on_segment(s.a, t, eps)) candidates[n++] = s.a;
    if (on_segment(s.b, t, eps)) candidates[n++] = s.b;
    if (on_segment(t.a, s, eps)) candidates[n++] = t.a;
    if (on_segment(t.b, s, eps)) candidates[n++] = t.b;
    if (n != 0) {
        auto ends = std::minmax_element(candidates.begin(), candidates.begin() + static_cast<std::ptrdiff_t>(n));
        if (*ends.first == *ends.second) return {SegmentIntersectionKind::Point, {to_real(*ends.first)}};
        return {SegmentIntersectionKind::Overlap, {to_real(*ends.first), to_real(*ends.second)}};
    }
    int a = orientation(s.a, s.b, t.a, eps), b = orientation(s.a, s.b, t.b, eps);
    int c = orientation(t.a, t.b, s.a, eps), d = orientation(t.a, t.b, s.b, eps);
    if (!(a * b < 0 && c * d < 0)) return {};
    C ux = C(s.b.x) - C(s.a.x), uy = C(s.b.y) - C(s.a.y);
    C vx = C(t.b.x) - C(t.a.x), vy = C(t.b.y) - C(t.a.y);
    C wx = C(t.a.x) - C(s.a.x), wy = C(t.a.y) - C(s.a.y);
    C determinant = ux * vy - uy * vx;
    if (determinant == 0) return {};
    long double ratio = static_cast<long double>(wx * vy - wy * vx) / static_cast<long double>(determinant);
    return {SegmentIntersectionKind::Point, {{static_cast<long double>(s.a.x) + ratio * static_cast<long double>(ux), static_cast<long double>(s.a.y) + ratio * static_cast<long double>(uy)}}};
}
}
#endif


// SPDX-License-Identifier: CC0-1.0
// Verify I/O: Verify/current/common/fast_io.hpp
// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef REINSIRK_VERIFY_FAST_IO_DEFINED
#define REINSIRK_VERIFY_FAST_IO_DEFINED

// VerifyInput(file=stdin): 非所有のFILEから読む。別の入力方式と混用しない。
// >> 整数: 64bit以下の十進整数を読む。検査省略時は値域内の正しい入力を前提とする。
// >> char/string: 空白を飛ばし、1文字／空白までの文字列を読む。
// >> 浮動小数点: 十進文字列を変換する。通常のCロケールで使う。
// bool / !: 直前までの読み取りが成功しているか／失敗したかを返す。
class VerifyInput {
    static constexpr std::size_t buffer_size = 1 << 17;
    std::FILE* file_;
    std::array<unsigned char, buffer_size> buffer_;
    std::size_t position_ = 0, size_ = 0;
    bool good_ = true;

    int peek() {
        if (position_ == size_) {
            size_ = std::fread(buffer_.data(), 1, buffer_.size(), file_);
            position_ = 0;
            if (size_ == 0) return EOF;
        }
        return buffer_[position_];
    }

    int skip_space() {
        int c;
        while ((c = peek()) != EOF && c <= ' ') ++position_;
        if (c == EOF) good_ = false;
        return c;
    }

public:
    explicit VerifyInput(std::FILE* file = stdin) : file_(file) {}
    VerifyInput(const VerifyInput&) = delete;
    VerifyInput& operator=(const VerifyInput&) = delete;

    explicit operator bool() const noexcept { return good_; }
    bool operator!() const noexcept { return !good_; }

    template<class T>
        requires (std::is_integral_v<T> && sizeof(T) <= sizeof(std::uint64_t)
                  && !std::is_same_v<T, char>)
    VerifyInput& operator>>(T& result) {
        if (!good_) return *this;
        int c = skip_space();
        if (c == EOF) return *this;
        bool negative = false;
        if (c == '-' || c == '+') {
            negative = c == '-';
            ++position_;
            c = peek();
        }
        if (c == EOF) {
            good_ = false;
            return *this;
        }
#ifndef SKIP_RUNTIME_CHECKS
        if (c < '0' || c > '9') {
            good_ = false;
            return *this;
        }
        // 先頭の0を除けば、20桁以下の末尾直前の値はuint64_tに収まる。
        while (c == '0') {
            ++position_;
            c = peek();
        }
        std::uint64_t previous = 0;
        unsigned last_digit = 0;
        std::size_t digits = 0;
#endif
        std::uint64_t value = 0;
        while (c >= '0' && c <= '9') {
#ifndef SKIP_RUNTIME_CHECKS
            previous = value;
            last_digit = static_cast<unsigned>(c - '0');
            if (digits <= 20) ++digits;
#endif
            value = value * 10 + static_cast<unsigned>(c - '0');
            ++position_;
            c = peek();
        }
#ifndef SKIP_RUNTIME_CHECKS
        constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
        if ((c != EOF && c > ' ') || digits > 20
            || (digits == 20 && (previous > maximum / 10
                || (previous == maximum / 10 && last_digit > maximum % 10)))) {
            good_ = false;
            return *this;
        }
        std::uint64_t limit = static_cast<std::uint64_t>(std::numeric_limits<T>::max());
        if constexpr (std::is_signed_v<T>) limit += negative;
        else if (negative && value != 0) good_ = false;
        if (value > limit) good_ = false;
        if (!good_) return *this;
#endif
        if constexpr (std::is_signed_v<T>) {
            constexpr std::uint64_t minimum_magnitude =
                static_cast<std::uint64_t>(std::numeric_limits<T>::max()) + 1;
            // -(最小値)は評価せず、絶対値が最大値+1のときだけ最小値を使う。
            result = negative
                ? (value == minimum_magnitude ? std::numeric_limits<T>::min()
                                  : static_cast<T>(-static_cast<T>(value)))
                : static_cast<T>(value);
        } else result = static_cast<T>(value);
        return *this;
    }

    VerifyInput& operator>>(char& result) {
        if (!good_) return *this;
        const int c = skip_space();
        if (c != EOF) {
            result = static_cast<char>(c);
            ++position_;
        }
        return *this;
    }

    VerifyInput& operator>>(std::string& result) {
        if (!good_) return *this;
        int c = skip_space();
        if (c == EOF) return *this;
        result.clear();
        do {
            result.push_back(static_cast<char>(c));
            ++position_;
            c = peek();
        } while (c != EOF && c > ' ');
        return *this;
    }

    template<class T> requires std::is_floating_point_v<T>
    VerifyInput& operator>>(T& result) {
        std::string token;
        *this >> token;
        if (!good_) return *this;
#ifndef SKIP_RUNTIME_CHECKS
        char* end = nullptr;
        const long double value = std::strtold(token.c_str(), &end);
        if (end != token.c_str() + token.size()) good_ = false;
        else result = static_cast<T>(value);
#else
        result = static_cast<T>(std::strtold(token.c_str(), nullptr));
#endif
        return *this;
    }
};

// VerifyOutput(file=stdout): 非所有のFILEへ書く。破棄時にflushし、FILEは閉じない。
// << 整数/char/文字列/浮動小数点: 値を出力する。整数は64bit以下に対応する。
// fixed(digits): 浮動小数点を小数点以下digits桁で出力する（digits>=0）。
// flush(): バッファの内容をFILEへ渡す。bool / !で書き込み失敗を確認できる。
class VerifyOutput {
    static constexpr std::size_t buffer_size = 1 << 17;
    std::FILE* file_;
    std::array<char, buffer_size> buffer_;
    std::size_t size_ = 0;
    int precision_ = 6;
    bool fixed_ = false, good_ = true;

    void write(std::string_view text) {
        while (!text.empty() && good_) {
            if (size_ == buffer_.size()) flush();
            const auto count = std::min(text.size(), buffer_.size() - size_);
            std::memcpy(buffer_.data() + size_, text.data(), count);
            size_ += count;
            text.remove_prefix(count);
        }
    }

public:
    explicit VerifyOutput(std::FILE* file = stdout) : file_(file) {}
    VerifyOutput(const VerifyOutput&) = delete;
    VerifyOutput& operator=(const VerifyOutput&) = delete;
    ~VerifyOutput() { flush(); }

    explicit operator bool() const noexcept { return good_; }
    bool operator!() const noexcept { return !good_; }

    VerifyOutput& fixed(int digits) {
        if (digits < 0) throw std::invalid_argument("negative output precision");
        fixed_ = true;
        precision_ = digits;
        return *this;
    }

    void flush() noexcept {
        std::size_t written = 0;
        while (written < size_ && good_) {
            const auto count = std::fwrite(buffer_.data() + written, 1, size_ - written, file_);
            if (count == 0) good_ = false;
            written += count;
        }
        size_ = 0;
    }

    template<class T>
        requires (std::is_integral_v<T> && sizeof(T) <= sizeof(std::uint64_t)
                  && !std::is_same_v<T, char>)
    VerifyOutput& operator<<(T value) {
        if constexpr (std::is_same_v<T, bool>) return *this << static_cast<char>('0' + value);
        else {
            char digits[std::numeric_limits<std::uint64_t>::digits10 + 3];
            const auto end = std::to_chars(digits, digits + sizeof(digits), value).ptr;
            write(std::string_view(digits, static_cast<std::size_t>(end - digits)));
            return *this;
        }
    }

    VerifyOutput& operator<<(char value) {
        if (size_ == buffer_.size()) flush();
        if (good_) buffer_[size_++] = value;
        return *this;
    }

    VerifyOutput& operator<<(std::string_view value) { write(value); return *this; }
    VerifyOutput& operator<<(const std::string& value) { write(value); return *this; }
    VerifyOutput& operator<<(const char* value) { write(value); return *this; }

    template<class T> requires std::is_floating_point_v<T>
    VerifyOutput& operator<<(T value) {
        char local[128];
        const char* format = fixed_ ? "%.*Lf" : "%.*Lg";
        const int count = std::snprintf(local, sizeof(local), format, precision_,
                                        static_cast<long double>(value));
        if (count < 0) good_ = false;
        else if (static_cast<std::size_t>(count) < sizeof(local))
            write(std::string_view(local, static_cast<std::size_t>(count)));
        else {
            std::vector<char> text(static_cast<std::size_t>(count) + 1);
            std::snprintf(text.data(), text.size(), format, precision_, static_cast<long double>(value));
            write(std::string_view(text.data(), static_cast<std::size_t>(count)));
        }
        return *this;
    }
};

inline VerifyInput verify_in;
inline VerifyOutput verify_out;
#endif

#include <cstdint>

int main() {
    geo::Segment<std::int64_t> a, b;
    verify_in >> a.a.x >> a.a.y >> a.b.x >> a.b.y;
    verify_in >> b.a.x >> b.a.y >> b.b.x >> b.b.y;
    verify_out << (geo::intersects(a, b) ? "Yes" : "No") << '\n';
}
