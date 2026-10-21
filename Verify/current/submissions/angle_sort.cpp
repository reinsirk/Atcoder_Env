#define SKIP_RUNTIME_CHECKS
// Verify: https://judge.yosupo.jp/problem/sort_points_by_argument
// Submission: https://judge.yosupo.jp/submission/411974

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

#ifndef SIRKIT_ANGLE_ARITHMETIC_DEFINED
#define SIRKIT_ANGLE_ARITHMETIC_DEFINED
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

#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
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

#ifndef SIRKIT_ANGLE_CROSS_SIGN_DEFINED
#define SIRKIT_ANGLE_CROSS_SIGN_DEFINED
// a,b の外積の符号を -1/0/1 で返す。int64_t 全域で厳密、O(1)。
inline constexpr int angle_cross_sign(
    const std::pair<std::int64_t, std::int64_t>& a,
    const std::pair<std::int64_t, std::int64_t>& b) noexcept {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
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

#ifndef SIRKIT_ANGLE_STANDARD_LESS_WIDE_DEFINED
#define SIRKIT_ANGLE_STANDARD_LESS_WIDE_DEFINED
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

#ifndef SIRKIT_ARCCMP_LESS_DEFINED
#define SIRKIT_ARCCMP_LESS_DEFINED
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
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
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

#ifndef SIRKIT_ARCCMP_G_DEFINED
#define SIRKIT_ARCCMP_G_DEFINED
// 偏角で a>b を返す。零は降順末尾、同方向は等値、O(1)。
inline constexpr bool arccmp_g(
    const std::pair<std::int64_t, std::int64_t>& a,
    const std::pair<std::int64_t, std::int64_t>& b) noexcept {
    return arccmp_less(b, a);
}
#endif

#ifndef SIRKIT_ARCCMP_LESS_EQ_DEFINED
#define SIRKIT_ARCCMP_LESS_EQ_DEFINED
// 零を先頭とする偏角で a<=b を返す。同方向は真、O(1)。
inline constexpr bool arccmp_less_eq(
    const std::pair<std::int64_t, std::int64_t>& a,
    const std::pair<std::int64_t, std::int64_t>& b) noexcept {
    return !arccmp_less(b, a);
}
#endif

#ifndef SIRKIT_ARCCMP_G_EQ_DEFINED
#define SIRKIT_ARCCMP_G_EQ_DEFINED
// 零を先頭とする偏角で a>=b を返す。同方向は真、O(1)。
inline constexpr bool arccmp_g_eq(
    const std::pair<std::int64_t, std::int64_t>& a,
    const std::pair<std::int64_t, std::int64_t>& b) noexcept {
    return !arccmp_less(a, b);
}
#endif

#ifndef SIRKIT_SAME_ANGLE_DEFINED
#define SIRKIT_SAME_ANGLE_DEFINED
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

#ifndef SIRKIT_ANGLE_LESS_DEFINED
#define SIRKIT_ANGLE_LESS_DEFINED
// std::sort 用の昇順偏角比較器。零は先頭、同方向は等値、O(1)。
struct AngleLess {
    constexpr bool operator()(const std::pair<std::int64_t, std::int64_t>& a,
                              const std::pair<std::int64_t, std::int64_t>& b) const noexcept {
        return arccmp_less(a, b);
    }
};
#endif

#ifndef SIRKIT_ANGLE_GREATER_DEFINED
#define SIRKIT_ANGLE_GREATER_DEFINED
// std::sort 用の降順偏角比較器。零は末尾、同方向は等値、O(1)。
struct AngleGreater {
    constexpr bool operator()(const std::pair<std::int64_t, std::int64_t>& a,
                              const std::pair<std::int64_t, std::int64_t>& b) const noexcept {
        return arccmp_g(a, b);
    }
};
#endif

#ifndef SIRKIT_ANGLE_SORT_LARGE_DEFINED
#define SIRKIT_ANGLE_SORT_LARGE_DEFINED
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
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
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

#ifndef SIRKIT_ANGLE_SORT_DEFINED
#define SIRKIT_ANGLE_SORT_DEFINED
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

#ifndef SIRKIT_ANGLE_SORT_INDICES_DEFINED
#define SIRKIT_ANGLE_SORT_INDICES_DEFINED
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

#ifndef SIRKIT_VERIFY_FAST_IO_DEFINED
#define SIRKIT_VERIFY_FAST_IO_DEFINED

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

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

int main() {
    int n{};
    verify_in >> n;
    std::vector<std::pair<std::int64_t, std::int64_t>> points(n);
    for (auto& [x, y] : points) verify_in >> x >> y;
    angle_sort(points);
    // ライブラリの[0,2π)順を、問題の(-π,π]順に合わせる。
    const auto split = std::find_if(points.begin(), points.end(), [](const auto& p) { return p.second < 0; });
    std::rotate(points.begin(), split, points.end());
    for (const auto& [x, y] : points) verify_out << x << ' ' << y << '\n';
}
