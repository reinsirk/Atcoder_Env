#define SKIP_RUNTIME_CHECKS
// Verify: https://atcoder.jp/contests/abc012/tasks/abc012_4
// Submission: https://atcoder.jp/contests/abc012/submissions/80244912

// Library: Graph/Floyd-Warshall.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://zenn.dev/reputeless/books/standard-cpp-for-competitive-programming/viewer/floyd-warshall

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>
#include <type_traits>
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
#include <immintrin.h>
#endif

#ifndef SIRKIT_INF_DEFINED
#define SIRKIT_INF_DEFINED
inline constexpr std::int64_t INF = std::int64_t{1} << 60;
#endif

// FloydWarshall(distances): 正方行列を全点対最短距離で上書きし、グラフ内に負閉路があればtrue。O(V^3)時間。
// FloydWarshall(distances, next): 距離に加え、経路の次頂点表nextを作る。経路更新・O(V^2)追加領域はこの版のみ。
// FloydWarshallPath(next, from, to): 次頂点表から頂点列をO(経路長)で復元。未到達・負閉路の影響下なら空。
// 入力はINFが未接続、有限重みは(-INF, INF)。対角はmin(入力, 0)とする。
// 出力は未到達INF、負閉路を経由できる組だけ-INF。有限距離が(-INF, INF)外ならoverflow_error。
// 算術範囲の算出を兼ねて初期行列を走査する。SKIP_RUNTIME_CHECKSで形状・入力値・頂点の検査を除く。
// 通常版の追加領域はO(V)。大きな重みで広幅演算が必要な場合はO(V^2)。例外時の行列の内容は未規定。
#ifndef SIRKIT_FLOYD_WARSHALL_DEFINED
#define SIRKIT_FLOYD_WARSHALL_DEFINED
namespace reinsirk::detail {

#if defined(__SIZEOF_INT128__) && (defined(__GNUC__) || defined(__clang__))
using FloydWarshallWide = __int128_t;
#else
// 必要なのは最大約92ビット。符号付き上位語と符号なし下位語で標準C++でも扱う。
struct FloydWarshallWide
{
    std::int64_t high = 0;
    std::uint64_t low = 0;
    constexpr FloydWarshallWide() = default;
    constexpr FloydWarshallWide(std::int64_t value)
        : high(value < 0 ? -1 : 0), low(static_cast<std::uint64_t>(value)) {}
    constexpr FloydWarshallWide(std::int64_t upper, std::uint64_t lower)
        : high(upper), low(lower) {}
    friend constexpr FloydWarshallWide operator+(FloydWarshallWide a,
                                                        FloydWarshallWide b)
    {
        const auto lower = a.low + b.low;
        return {a.high + b.high + (lower < a.low), lower};
    }
    friend constexpr FloydWarshallWide operator-(FloydWarshallWide value)
    {
        const auto lower = std::uint64_t{0} - value.low;
        return {~value.high + (lower == 0), lower};
    }
    friend constexpr bool operator<(FloydWarshallWide a, FloydWarshallWide b)
    {
        return a.high < b.high || (a.high == b.high && a.low < b.low);
    }
    friend constexpr bool operator==(FloydWarshallWide, FloydWarshallWide) = default;
    explicit constexpr operator std::int64_t() const { return std::bit_cast<std::int64_t>(low); }
};
#endif

// 広幅実装でも下限の絶対値は単純路の最大絶対値より大きい。
inline FloydWarshallWide FloydWarshallLimit(std::size_t n)
{
#if defined(__SIZEOF_INT128__) && (defined(__GNUC__) || defined(__clang__))
    return static_cast<FloydWarshallWide>(n) * INF;
#else
    return {static_cast<std::int64_t>(n >> 4), static_cast<std::uint64_t>(n) << 60};
#endif
}

#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
template <bool RestorePath, bool HasNegative, bool Dense>
[[gnu::target("avx2")]]
inline void FloydWarshallRelaxAvx2(std::vector<std::vector<std::int64_t>> &distance,
                                 std::int64_t unreachable, std::int64_t lower,
                                 std::vector<std::vector<int>> *next)
{
    const std::size_t n = distance.size();
    const __m256i v_unreachable = _mm256_set1_epi64x(unreachable);
    const __m256i v_lower = _mm256_set1_epi64x(lower);
    for (std::size_t via = 0; via < n; ++via)
    {
        const auto *through = distance[via].data();
        for (std::size_t from = 0; from < n; ++from)
        {
            auto *row = distance[from].data();
            const auto prefix = row[via];
            if (prefix == unreachable)
                continue;
            const __m256i v_prefix = _mm256_set1_epi64x(prefix);
            int *path = nullptr;
            int following = 0;
            if constexpr (RestorePath)
            {
                path = (*next)[from].data();
                following = path[via];
            }
            std::size_t to = 0;
            for (; to + 4 <= n; to += 4)
            {
                const __m256i values = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(through + to));
                const __m256i old = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(row + to));
                __m256i candidate = _mm256_add_epi64(v_prefix, values);
                if constexpr (HasNegative)
                    candidate = _mm256_blendv_epi8(candidate, v_lower, _mm256_cmpgt_epi64(v_lower, candidate));
                __m256i improve = _mm256_cmpgt_epi64(old, candidate);
                // 未到達laneの加算結果は捨て、到達済みのlaneだけ更新する。
                if constexpr (!Dense)
                    improve = _mm256_andnot_si256(_mm256_cmpeq_epi64(values, v_unreachable), improve);
                _mm256_storeu_si256(reinterpret_cast<__m256i *>(row + to), _mm256_blendv_epi8(old, candidate, improve));
                if constexpr (RestorePath)
                {
                    const __m128 lo = _mm_castsi128_ps(_mm256_castsi256_si128(improve));
                    const __m128 hi = _mm_castsi128_ps(_mm256_extracti128_si256(improve, 1));
                    const __m128i mask = _mm_castps_si128(_mm_shuffle_ps(lo, hi, _MM_SHUFFLE(2, 0, 2, 0)));
                    const __m128i previous = _mm_loadu_si128(reinterpret_cast<const __m128i *>(path + to));
                    _mm_storeu_si128(reinterpret_cast<__m128i *>(path + to),
                                     _mm_blendv_epi8(previous, _mm_set1_epi32(following), mask));
                }
            }
            for (; to < n; ++to)
            {
                if constexpr (!Dense)
                    if (through[to] == unreachable)
                        continue;
                auto candidate = prefix + through[to];
                if constexpr (HasNegative)
                    candidate = std::max(candidate, lower);
                if (candidate < row[to])
                {
                    row[to] = candidate;
                    if constexpr (RestorePath)
                        path[to] = following;
                }
            }
        }
    }
}
#endif
template <bool RestorePath, bool HasNegative, bool FiniteRange = false, bool Dense = false, class Distance, bool UseSimd = false>
inline bool FloydWarshallRun(std::vector<std::vector<Distance>> &distance,
                                    Distance unreachable, Distance lower,
                                    std::vector<std::vector<int>> *next)
{
    const std::size_t n = distance.size();
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
    if constexpr (UseSimd && std::is_same_v<Distance, std::int64_t>)
        FloydWarshallRelaxAvx2<RestorePath, HasNegative, Dense>(distance, unreachable, lower, next);
    else
#endif
    {
        for (std::size_t via = 0; via < n; ++via)
        {
            const auto *through = distance[via].data();
            for (std::size_t from = 0; from < n; ++from)
            {
                auto *row = distance[from].data();
                const Distance prefix = row[via];
                if (prefix == unreachable)
                    continue;
                for (std::size_t to = 0; to < n; ++to)
                {
                    if constexpr (!Dense) if (through[to] == unreachable)
                        continue;
                    Distance candidate = prefix + through[to];
                    if constexpr (HasNegative)
                        candidate = std::max(candidate, lower);
                    if constexpr (RestorePath)
                    {
                        if (candidate < row[to])
                        {
                            row[to] = candidate;
                            (*next)[from][to] = (*next)[from][via];
                        }
                    }
                    else
                        row[to] = std::min(row[to], candidate);
                }
            }
        }
    }
    if constexpr (FiniteRange) return false;
    std::vector<std::size_t> negative;
    for (std::size_t vertex = 0; vertex < n; ++vertex)
        if (distance[vertex][vertex] < Distance{0})
            negative.push_back(vertex);
    const Distance affected = lower + Distance{-1};
    for (const std::size_t via : negative)
    {
        // 同じ強連結成分の負閉路は一度の伝播で足りる。
        if (distance[via][via] == affected)
            continue;
        for (std::size_t from = 0; from < n; ++from)
        {
            if (distance[from][via] == unreachable)
                continue;
            for (std::size_t to = 0; to < n; ++to)
                if (!(distance[via][to] == unreachable))
                {
                    distance[from][to] = affected;
                    if constexpr (RestorePath)
                        (*next)[from][to] = -1;
                }
        }
    }
    for (auto &row : distance)
        for (auto &value : row)
        {
            if (value == unreachable)
                value = Distance{INF};
            else if (value == affected)
                value = Distance{-INF};
            else if (!(Distance{-INF} < value) || !(value < Distance{INF}))
                throw std::overflow_error("FloydWarshall: finite distance collides with INF/-INF");
        }
    return !negative.empty();
}

template <bool RestorePath, bool UseSimd = false>
inline bool FloydWarshallImpl(std::vector<std::vector<std::int64_t>> &distance,
                                     std::vector<std::vector<int>> *next)
{
    const std::size_t n = distance.size();
    if (n > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        throw std::length_error("FloydWarshall: too many vertices");
    if constexpr (RestorePath)
        next->assign(n, std::vector<int>(n, -1));
    if (n == 0)
        return false;
    if (n == 1)
    {
#ifndef SKIP_RUNTIME_CHECKS
        if (distance[0].size() != 1)
            throw std::invalid_argument("FloydWarshall: matrix must be square");
        if (distance[0][0] != INF && (distance[0][0] <= -INF || distance[0][0] >= INF))
            throw std::out_of_range("FloydWarshall: weight must be in (-INF, INF)");
#endif
        const bool negative = distance[0][0] < 0;
        distance[0][0] = negative ? -INF : 0;
        if constexpr (RestorePath)
            (*next)[0][0] = negative ? -1 : 0;
        return negative;
    }
    std::int64_t maximum = 0;
    bool has_negative = false;
    bool dense = true;
    constexpr auto unreachable = std::numeric_limits<std::int64_t>::max();
    for (std::size_t from = 0; from < n; ++from)
    {
        auto &row = distance[from];
#ifndef SKIP_RUNTIME_CHECKS
        if (row.size() != n)
            throw std::invalid_argument("FloydWarshall: matrix must be square");
#endif
        for (std::size_t to = 0; to < n; ++to)
        {
            auto &value = row[to];
#ifndef SKIP_RUNTIME_CHECKS
            if (value != INF && (value <= -INF || value >= INF))
                throw std::out_of_range("FloydWarshall: weight must be in (-INF, INF)");
#endif
            if (from == to)
                value = std::min(value, std::int64_t{0});
            if (value == INF) {
                dense = false;
                continue;
            }
            maximum = std::max(maximum, value < 0 ? -value : value);
            has_negative = has_negative || value < 0;
            if constexpr (RestorePath)
                (*next)[from][to] = static_cast<int>(to);
        }
    }
    // 非負で最長単純路もINF未満なら、未接続値と出力の変換が要らない。
    if (!has_negative && maximum <= (INF - 1) / static_cast<std::int64_t>(n)) {
        if (dense) return FloydWarshallRun<RestorePath, false, true, true, std::int64_t, UseSimd>(distance, INF, std::int64_t{0}, next);
        return FloydWarshallRun<RestorePath, false, true, false, std::int64_t, UseSimd>(distance, INF, std::int64_t{0}, next);
    }
    for (auto& row : distance)
        for (auto& value : row)
            if (value == INF) value = unreachable;
    // 単純路はn-1辺以下。下限は-n*maximumで、負閉路での際限ない減少を抑える。
    if (maximum <= unreachable / (2 * static_cast<std::int64_t>(n)))
    {
        const auto lower = -static_cast<std::int64_t>(n) * maximum;
        if (has_negative)
            return FloydWarshallRun<RestorePath, true, false, false, std::int64_t, UseSimd>(distance, unreachable, lower, next);
        return FloydWarshallRun<RestorePath, false, false, false, std::int64_t, UseSimd>(distance, unreachable, lower, next);
    }

    using Wide = FloydWarshallWide;
    const Wide wide_unreachable = FloydWarshallLimit(n) + Wide{1};
    std::vector<std::vector<Wide>> wide(n, std::vector<Wide>(n));
    for (std::size_t from = 0; from < n; ++from)
        for (std::size_t to = 0; to < n; ++to)
            wide[from][to] = distance[from][to] == unreachable ? wide_unreachable : Wide{distance[from][to]};
    const bool negative = has_negative
        ? FloydWarshallRun<RestorePath, true>(wide, wide_unreachable, -FloydWarshallLimit(n), next)
        : FloydWarshallRun<RestorePath, false>(wide, wide_unreachable, -FloydWarshallLimit(n), next);
    for (std::size_t from = 0; from < n; ++from)
        for (std::size_t to = 0; to < n; ++to)
            distance[from][to] = static_cast<std::int64_t>(wide[from][to]);
    return negative;
}

#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
template <bool RestorePath>
[[gnu::noinline]] inline bool FloydWarshallVectorized(
    std::vector<std::vector<std::int64_t>> &distance, std::vector<std::vector<int>> *next)
{
    return FloydWarshallImpl<RestorePath, true>(distance, next);
}
#endif

} // namespace reinsirk::detail

inline bool FloydWarshall(std::vector<std::vector<std::int64_t>> &distances)
{
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__)) && !(defined(__clang__) && defined(__AVX2__))
#if defined(__clang__)
    constexpr std::size_t threshold = 32;
#else
    constexpr std::size_t threshold = 8;
#endif
    if (distances.size() >= threshold && __builtin_cpu_supports("avx2"))
        return reinsirk::detail::FloydWarshallVectorized<false>(distances, nullptr);
#endif
    return reinsirk::detail::FloydWarshallImpl<false>(distances, nullptr);
}

inline bool FloydWarshall(std::vector<std::vector<std::int64_t>> &distances,
                          std::vector<std::vector<int>> &next)
{
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
#if defined(__clang__)
    constexpr std::size_t threshold = 16;
#else
    constexpr std::size_t threshold = 8;
#endif
    if (distances.size() >= threshold && __builtin_cpu_supports("avx2"))
        return reinsirk::detail::FloydWarshallVectorized<true>(distances, &next);
#endif
    return reinsirk::detail::FloydWarshallImpl<true>(distances, &next);
}

inline std::vector<int> FloydWarshallPath(const std::vector<std::vector<int>> &next,
                                        int from, int to)
{
    const std::size_t n = next.size();
#ifndef SKIP_RUNTIME_CHECKS
    if (from < 0 || to < 0 || static_cast<std::size_t>(from) >= n || static_cast<std::size_t>(to) >= n)
        throw std::out_of_range("FloydWarshallPath: invalid vertex");
#endif
    std::vector<int> path;
    const auto target = static_cast<std::size_t>(to);
    for (std::size_t steps = 0; steps <= n; ++steps)
    {
        const auto current = static_cast<std::size_t>(from);
#ifndef SKIP_RUNTIME_CHECKS
        if (next[current].size() != n)
            throw std::invalid_argument("FloydWarshallPath: table must be square");
#endif
        const int following = next[current][target];
        if (following == -1)
            return {};
        path.push_back(from);
        if (from == to)
            return path;
#ifndef SKIP_RUNTIME_CHECKS
        if (following < 0 || static_cast<std::size_t>(following) >= n)
            throw std::out_of_range("FloydWarshallPath: invalid next vertex");
#endif
        from = following;
    }
    throw std::invalid_argument("FloydWarshallPath: cyclic next table");
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
#include <vector>

int main() {
    int n{}, m{}; verify_in >> n >> m;
    std::vector distance(n, std::vector<std::int64_t>(n, INF));
    for (int v = 0; v < n; ++v) distance[v][v] = 0;
    while (m--) {
        int a{}, b{}; std::int64_t time{}; verify_in >> a >> b >> time;
        distance[a - 1][b - 1] = distance[b - 1][a - 1] = time;
    }
    FloydWarshall(distance);
    std::int64_t answer = INF;
    for (const auto& row : distance)
        answer = std::min(answer, *std::max_element(row.begin(), row.end()));
    verify_out << answer << '\n';
}
