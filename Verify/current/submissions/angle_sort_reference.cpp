// Verify: ローカル独立検査

// Library: Geometry/偏角ソート.cpp
// SPDX-License-Identifier: CC0-1.0

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <utility>
#include <vector>

// 偏角は+xから反時計回りの[0,2π)。零は昇順先頭、同方向の長さ順は任意。
// arccmp_less/arccmp_g: 厳密な昇順/降順比較。std::sortに渡せる。O(1)。
// arccmp_less_eq/arccmp_g_eq: 非厳密比較。std::sortの比較器には使わない。O(1)。
// angle_cross_sign/same_angle: int64_t全域の外積符号/同方向判定。O(1)。
// AngleLess/AngleGreater: 昇順/降順の比較器オブジェクト。O(1)。
// angle_sort: 配列をその場でソート。angle_sort_indices: 入力不変で添字順を返す。O(n log n)。
// いずれもint64_t全域で有効なため入力検査は不要。SKIP_RUNTIME_CHECKSで動作は変わらない。

#ifndef REINSIRK_ANGLE_ARITHMETIC_DEFINED
#define REINSIRK_ANGLE_ARITHMETIC_DEFINED
namespace reinsirk::detail {
struct AngleArithmetic {
    static constexpr std::uint64_t magnitude(std::int64_t x) noexcept {
        const auto u = static_cast<std::uint64_t>(x);
        return x < 0 ? std::uint64_t{0} - u : u;
    }

    struct Product {
        std::uint64_t hi, lo;
    };

    static constexpr Product multiply(std::uint64_t x, std::uint64_t y) noexcept {
        constexpr std::uint64_t mask = 0xffffffffu;
        const std::uint64_t x0 = x & mask, x1 = x >> 32;
        const std::uint64_t y0 = y & mask, y1 = y >> 32;
        const std::uint64_t low = x0 * y0;
        const std::uint64_t middle = x1 * y0 + (low >> 32);
        const std::uint64_t carry = (middle & mask) + x0 * y1;
        return {x1 * y1 + (middle >> 32) + (carry >> 32),
                (carry << 32) | (low & mask)};
    }

    static constexpr int compare_product(Product x, Product y) noexcept {
        if (x.hi != y.hi) return x.hi > y.hi ? 1 : -1;
        return (x.lo > y.lo) - (x.lo < y.lo);
    }

    static constexpr bool lower_half(const std::pair<std::int64_t, std::int64_t>& p) noexcept {
        return p.second < 0 || (p.second == 0 && p.first < 0);
    }

    static constexpr bool zero(const std::pair<std::int64_t, std::int64_t>& p) noexcept {
        return p.first == 0 && p.second == 0;
    }

#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(REINSIRK_FORCE_STANDARD_ARITHMETIC)
    [[gnu::noinline]] static constexpr bool less_wide(
        const std::pair<std::int64_t, std::int64_t>& a,
        const std::pair<std::int64_t, std::int64_t>& b) noexcept {
        const __int128 p = static_cast<__int128>(a.first) * b.second;
        const __int128 q = static_cast<__int128>(a.second) * b.first;
        return p > q;
    }
#endif
};
} // namespace reinsirk::detail
#endif

#ifndef REINSIRK_ANGLE_CROSS_SIGN_DEFINED
#define REINSIRK_ANGLE_CROSS_SIGN_DEFINED
// a,b の外積の符号を -1/0/1 で返す。int64_t 全域で厳密、O(1)。
inline constexpr int angle_cross_sign(
    const std::pair<std::int64_t, std::int64_t>& a,
    const std::pair<std::int64_t, std::int64_t>& b) noexcept {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(REINSIRK_FORCE_STANDARD_ARITHMETIC)
    const __int128 p = static_cast<__int128>(a.first) * b.second;
    const __int128 q = static_cast<__int128>(a.second) * b.first;
    return (p > q) - (p < q);
#else
    const bool p_zero = a.first == 0 || b.second == 0;
    const bool q_zero = a.second == 0 || b.first == 0;
    const bool p_negative = !p_zero && ((a.first < 0) != (b.second < 0));
    const bool q_negative = !q_zero && ((a.second < 0) != (b.first < 0));
    if (p_negative != q_negative) return p_negative ? -1 : 1;
    const auto x = reinsirk::detail::AngleArithmetic::magnitude(a.first);
    const auto y = reinsirk::detail::AngleArithmetic::magnitude(b.second);
    const auto z = reinsirk::detail::AngleArithmetic::magnitude(a.second);
    const auto w = reinsirk::detail::AngleArithmetic::magnitude(b.first);
    int cmp;
    if ((x | y | z | w) <= std::uint64_t{0xffffffffu}) {
        const auto p = x * y, q = z * w;
        cmp = (p > q) - (p < q);
    } else {
        cmp = reinsirk::detail::AngleArithmetic::compare_product(
            reinsirk::detail::AngleArithmetic::multiply(x, y),
            reinsirk::detail::AngleArithmetic::multiply(z, w));
    }
    return p_negative ? -cmp : cmp;
#endif
}
#endif

#ifndef REINSIRK_ANGLE_STANDARD_LESS_WIDE_DEFINED
#define REINSIRK_ANGLE_STANDARD_LESS_WIDE_DEFINED
namespace reinsirk::detail {
#if defined(__GNUC__) || defined(__clang__)
[[gnu::noinline]]
#endif
inline constexpr bool AngleStandardLessWide(
    const std::pair<std::int64_t, std::int64_t>& a,
    const std::pair<std::int64_t, std::int64_t>& b) noexcept {
    return angle_cross_sign(a, b) > 0;
}
} // namespace reinsirk::detail
#endif

#ifndef REINSIRK_ARCCMP_LESS_DEFINED
#define REINSIRK_ARCCMP_LESS_DEFINED
// [0,2π) の偏角で a<b を返す。零は先頭、同方向は等値、O(1)。
inline constexpr bool arccmp_less(
    const std::pair<std::int64_t, std::int64_t>& a,
    const std::pair<std::int64_t, std::int64_t>& b) noexcept {
    // 水平軸を符号だけで処理し、通常の比較から零判定を除く。
    if ((a.second == 0) | (b.second == 0)) {
        if (a.second == 0) {
            if (b.second == 0)
                return a.first == 0 ? b.first != 0 : a.first > 0 && b.first < 0;
            return a.first >= 0 || b.second < 0;
        }
        return b.first < 0 && a.second > 0;
    }
    const bool ah = a.second < 0;
    const bool bh = b.second < 0;
    if (ah != bh) return ah < bh;
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(REINSIRK_FORCE_STANDARD_ARITHMETIC)
    std::int64_t p, q;
    if (__builtin_mul_overflow(a.first, b.second, &p)
        || __builtin_mul_overflow(a.second, b.first, &q))
        return reinsirk::detail::AngleArithmetic::less_wide(a, b);
    return p > q;
#else
    constexpr std::uint64_t bias = std::uint64_t{1} << 31;
    const std::uint64_t bits = (static_cast<std::uint64_t>(a.first) + bias)
        | (static_cast<std::uint64_t>(a.second) + bias)
        | (static_cast<std::uint64_t>(b.first) + bias)
        | (static_cast<std::uint64_t>(b.second) + bias);
    if (bits <= std::uint64_t{0xffffffffu}) [[likely]] {
        const std::int64_t p = a.first * b.second;
        const std::int64_t q = a.second * b.first;
        return p > q;
    }
    return reinsirk::detail::AngleStandardLessWide(a, b);
#endif
}
#endif

#ifndef REINSIRK_ARCCMP_G_DEFINED
#define REINSIRK_ARCCMP_G_DEFINED
// 偏角で a>b を返す。零は降順末尾、同方向は等値、O(1)。
inline constexpr bool arccmp_g(
    const std::pair<std::int64_t, std::int64_t>& a,
    const std::pair<std::int64_t, std::int64_t>& b) noexcept {
    return arccmp_less(b, a);
}
#endif

#ifndef REINSIRK_ARCCMP_LESS_EQ_DEFINED
#define REINSIRK_ARCCMP_LESS_EQ_DEFINED
// 零を先頭とする偏角で a<=b を返す。同方向は真、O(1)。
inline constexpr bool arccmp_less_eq(
    const std::pair<std::int64_t, std::int64_t>& a,
    const std::pair<std::int64_t, std::int64_t>& b) noexcept {
    return !arccmp_less(b, a);
}
#endif

#ifndef REINSIRK_ARCCMP_G_EQ_DEFINED
#define REINSIRK_ARCCMP_G_EQ_DEFINED
// 零を先頭とする偏角で a>=b を返す。同方向は真、O(1)。
inline constexpr bool arccmp_g_eq(
    const std::pair<std::int64_t, std::int64_t>& a,
    const std::pair<std::int64_t, std::int64_t>& b) noexcept {
    return !arccmp_less(a, b);
}
#endif

#ifndef REINSIRK_SAME_ANGLE_DEFINED
#define REINSIRK_SAME_ANGLE_DEFINED
// a,b が同方向なら真。零は零とのみ同方向、長さを問わず O(1)。
inline constexpr bool same_angle(
    const std::pair<std::int64_t, std::int64_t>& a,
    const std::pair<std::int64_t, std::int64_t>& b) noexcept {
    if (reinsirk::detail::AngleArithmetic::zero(a) || reinsirk::detail::AngleArithmetic::zero(b))
        return reinsirk::detail::AngleArithmetic::zero(a) && reinsirk::detail::AngleArithmetic::zero(b);
    return reinsirk::detail::AngleArithmetic::lower_half(a) == reinsirk::detail::AngleArithmetic::lower_half(b)
        && angle_cross_sign(a, b) == 0;
}
#endif

#ifndef REINSIRK_ANGLE_LESS_DEFINED
#define REINSIRK_ANGLE_LESS_DEFINED
// std::sort 用の昇順偏角比較器。零は先頭、同方向は等値、O(1)。
struct AngleLess {
    constexpr bool operator()(const std::pair<std::int64_t, std::int64_t>& a,
                              const std::pair<std::int64_t, std::int64_t>& b) const noexcept {
        return arccmp_less(a, b);
    }
};
#endif

#ifndef REINSIRK_ANGLE_GREATER_DEFINED
#define REINSIRK_ANGLE_GREATER_DEFINED
// std::sort 用の降順偏角比較器。零は末尾、同方向は等値、O(1)。
struct AngleGreater {
    constexpr bool operator()(const std::pair<std::int64_t, std::int64_t>& a,
                              const std::pair<std::int64_t, std::int64_t>& b) const noexcept {
        return arccmp_g(a, b);
    }
};
#endif

#ifndef REINSIRK_ANGLE_SORT_LARGE_DEFINED
#define REINSIRK_ANGLE_SORT_LARGE_DEFINED
namespace reinsirk::detail {
#if defined(__GNUC__) || defined(__clang__)
[[gnu::noinline]]
#endif
inline void AngleSortLarge(std::vector<std::pair<std::int64_t, std::int64_t>>& p,
                       bool descending) {
    // 半平面の振り分け中に、積をint64_tで比較できる範囲かも集計する。
    std::uint64_t bits = 0;
    constexpr std::uint64_t bias = std::uint64_t{1} << 31;
    const auto middle = std::partition(p.begin(), p.end(), [&](const auto& point) {
        bits |= (static_cast<std::uint64_t>(point.first) + bias)
              | (static_cast<std::uint64_t>(point.second) + bias);
        return AngleArithmetic::lower_half(point) == descending;
    });
    // 零を外積だけの比較から分離し、等値関係の推移性を保つ。
    const auto upper_begin = descending ? middle : p.begin();
    const auto upper_end = descending ? p.end() : middle;
    const auto zero_end = std::partition(upper_begin, upper_end, [descending](const auto& point) {
        return AngleArithmetic::zero(point) != descending;
    });
    const auto first = descending ? p.begin() : zero_end;
    const auto last = descending ? zero_end : p.end();
    const auto sort_parts = [&](auto less) {
        if (descending) {
            const auto greater = [&](const auto& a, const auto& b) { return less(b, a); };
            std::sort(first, middle, greater);
            std::sort(middle, last, greater);
        } else {
            std::sort(first, middle, less);
            std::sort(middle, last, less);
        }
    };
    if (bits <= std::uint64_t{0xffffffffu}) {
        sort_parts([](const auto& a, const auto& b) {
            return a.first * b.second > a.second * b.first;
        });
    } else {
        sort_parts([](const auto& a, const auto& b) {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(REINSIRK_FORCE_STANDARD_ARITHMETIC)
            return static_cast<__int128>(a.first) * b.second
                 > static_cast<__int128>(a.second) * b.first;
#else
            return angle_cross_sign(a, b) > 0;
#endif
        });
    }
}
} // namespace reinsirk::detail
#endif

#ifndef REINSIRK_ANGLE_SORT_DEFINED
#define REINSIRK_ANGLE_SORT_DEFINED
// pをその場で偏角順にソート。descending=trueは降順、同角の順序は任意、O(n log n)。
inline void angle_sort(std::vector<std::pair<std::int64_t, std::int64_t>>& p,
                       bool descending = false) {
    if (p.size() < 32) {
        if (descending) std::sort(p.begin(), p.end(), AngleGreater{});
        else std::sort(p.begin(), p.end(), AngleLess{});
    } else {
        reinsirk::detail::AngleSortLarge(p, descending);
    }
}
#endif

#ifndef REINSIRK_ANGLE_SORT_INDICES_DEFINED
#define REINSIRK_ANGLE_SORT_INDICES_DEFINED
// p を変更せず偏角順の添字を返す。descending=true は降順、O(n log n)、追加領域 O(n)。
inline std::vector<std::size_t> angle_sort_indices(
    const std::vector<std::pair<std::int64_t, std::int64_t>>& p,
    bool descending = false) {
    std::vector<std::size_t> indices(p.size());
    std::iota(indices.begin(), indices.end(), std::size_t{0});
    if (descending) {
        std::sort(indices.begin(), indices.end(), [&p](std::size_t a, std::size_t b) {
            return arccmp_g(p[a], p[b]);
        });
    } else {
        std::sort(indices.begin(), indices.end(), [&p](std::size_t a, std::size_t b) {
            return arccmp_less(p[a], p[b]);
        });
    }
    return indices;
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
#include <limits>
#include <random>
#include <string>
#include <utility>
#include <vector>

int main() {
    using Point = std::pair<std::int64_t, std::int64_t>;
    std::vector<Point> points;
    for (int x = -8; x <= 8; ++x) for (int y = -8; y <= 8; ++y) points.push_back({x, y});
    auto argument = [](Point p) {
        if (p == Point{0, 0}) return -1.0L;
        long double result = std::atan2(static_cast<long double>(p.second), static_cast<long double>(p.first));
        return result < 0 ? result + 2 * std::acos(-1.0L) : result;
    };
    for (auto a : points) for (auto b : points) {
        const auto aa = argument(a), bb = argument(b);
        if (std::abs(aa - bb) > 1e-15L) assert(arccmp_less(a, b) == (aa < bb));
        assert(arccmp_g(a, b) == arccmp_less(b, a));
        assert(arccmp_less_eq(a, b) == !arccmp_less(b, a));
        assert(arccmp_g_eq(a, b) == !arccmp_less(a, b));
    }
    auto ascending = points; angle_sort(ascending);
    auto descending = points; angle_sort(descending, true);
    const auto indices = angle_sort_indices(points);
    const auto reverse_indices = angle_sort_indices(points, true);
    for (std::size_t i = 1; i < points.size(); ++i) {
        assert(!arccmp_less(ascending[i], ascending[i - 1]));
        assert(!arccmp_g(descending[i], descending[i - 1]));
        assert(!arccmp_less(points[indices[i]], points[indices[i - 1]]));
        assert(!arccmp_g(points[reverse_indices[i]], points[reverse_indices[i - 1]]));
    }
    const auto high = std::numeric_limits<std::int64_t>::max();
    assert(angle_cross_sign({high, high - 1}, {high - 1, high}) == 1);
    assert(angle_cross_sign({INT64_MIN, 0}, {0, INT64_MIN}) == 1);
    assert(same_angle({1, 1}, {high, high}));
    std::cout << "OK\n";
}
