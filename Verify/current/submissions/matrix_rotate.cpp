#define SKIP_RUNTIME_CHECKS
// Verify: https://atcoder.jp/contests/abc298/tasks/abc298_b
// Submission: https://atcoder.jp/contests/abc298/submissions/80281169

// Library: Algorithm/rotate.cpp
// SPDX-License-Identifier: CC0-1.0
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/matrix_transform.md
//
// API一覧
// [行列変換]
// 以下の全操作はvector<vector<T>>版とvector<string>版を持つ。
// 全APIは矩形が前提。コピー版のvector<vector<T>>はコピー構築可能なT（boolも可）、vector<string>版は文字盤面。
// コピー版は入力不変・独立した所有権を返す。要素のコピー・swapをO(1)とし、H行W列のコピー・回転はO(H+HW)。
// 形状確認は通常処理で参照する行だけ。SKIP_RUNTIME_CHECKSで除外。inplaceは例外までの変更が残り得る。
// 幅0の90度回転・転置は先頭以外の行を見ない。上下inplaceは奇数行数の中央行を見ない。
// rotate_90r(a): 時計回り90度の新行列。外側が空なら互換性のため{{}}、H本の空行なら{}。
// flip_j(a): 左右反転の新行列。外側が空なら互換性のため{{}}、H本の空行は維持。
// flip_i(a): 上下反転の新行列。外側が空なら互換性のため{{}}、H本の空行は維持。
// rotate_90l(a): 反時計回り90度の新行列。外側が空または幅0なら{}。
// rotate_180(a): 180度回転の新行列。外側が空なら{}、H本の空行は維持。
// rotate_quarter(a, int clockwise_turns): 時計回りの回数をmod4で正規化（負数・INT_MIN可）。外側が空なら{}。
// transpose(a): 転置した新行列。外側が空または幅0なら{}。
// flip_i_inplace(a): 行の交換で上下反転しvoidを返す。要素のswap不要。O(H)時間・追加O(1)、外側が空なら何もしない。
// flip_j_inplace(a): 各行を左右反転しvoidを返す。Tはswap可能。O(H+HW)時間・追加O(1)、外側が空なら何もしない。

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
#include <immintrin.h>
#endif


#ifndef SIRKIT_MATRIX_TRANSFORM_SIMD_DETAIL_DEFINED
#define SIRKIT_MATRIX_TRANSFORM_SIMD_DETAIL_DEFINED
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
namespace reinsirk::detail {
inline bool matrix_avx2_available() noexcept {
    return __builtin_cpu_supports("avx2");
}
template <int Mode, class Row, bool Strip = false>
[[gnu::target("avx2")]]
std::vector<Row> matrix_turn_avx2(const std::vector<Row>& a) {
    using Value = typename Row::value_type;
    const std::size_t h = a.size(), w = a.front().size();
    std::vector<Row> b;
    b.reserve(w);
    for (std::size_t j = 0; j < w; ++j) b.emplace_back(h, Value{});
    // 横長の大行列だけ列を分割し、出力行を続けて使う。
    for (std::size_t jb = 0; jb < w;) {
        const auto je = jb + (Strip ? std::min(std::size_t{32}, w - jb) : w - jb);
        std::size_t i = 0;
        for (; h - i >= 8; i += 8) {
            const Value* source[8];
            for (std::size_t r = 0; r < 8; ++r) {
                const auto& row = a[i + (Mode == 1 ? 7 - r : r)];
    #ifndef SKIP_RUNTIME_CHECKS
                if (jb == 0 && row.size() != w) throw std::invalid_argument("matrix must be rectangular");
    #endif
                source[r] = row.data();
            }
            const auto column = Mode == 1 ? h - i - 8 : i;
            std::size_t j = jb;
            for (; je - j >= 8; j += 8) {
                const auto r0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(source[0] + j));
                const auto r1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(source[1] + j));
                const auto r2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(source[2] + j));
                const auto r3 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(source[3] + j));
                const auto r4 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(source[4] + j));
                const auto r5 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(source[5] + j));
                const auto r6 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(source[6] + j));
                const auto r7 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(source[7] + j));
                const auto t0 = _mm256_unpacklo_epi32(r0, r1);
                const auto t1 = _mm256_unpackhi_epi32(r0, r1);
                const auto t2 = _mm256_unpacklo_epi32(r2, r3);
                const auto t3 = _mm256_unpackhi_epi32(r2, r3);
                const auto t4 = _mm256_unpacklo_epi32(r4, r5);
                const auto t5 = _mm256_unpackhi_epi32(r4, r5);
                const auto t6 = _mm256_unpacklo_epi32(r6, r7);
                const auto t7 = _mm256_unpackhi_epi32(r6, r7);
                const auto s0 = _mm256_unpacklo_epi64(t0, t2);
                const auto s1 = _mm256_unpackhi_epi64(t0, t2);
                const auto s2 = _mm256_unpacklo_epi64(t1, t3);
                const auto s3 = _mm256_unpackhi_epi64(t1, t3);
                const auto s4 = _mm256_unpacklo_epi64(t4, t6);
                const auto s5 = _mm256_unpackhi_epi64(t4, t6);
                const auto s6 = _mm256_unpacklo_epi64(t5, t7);
                const auto s7 = _mm256_unpackhi_epi64(t5, t7);
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(b[Mode == 3 ? w - 1 - j - 0 : j + 0].data() + column),
                    _mm256_permute2x128_si256(s0, s4, 0x20));
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(b[Mode == 3 ? w - 1 - j - 1 : j + 1].data() + column),
                    _mm256_permute2x128_si256(s1, s5, 0x20));
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(b[Mode == 3 ? w - 1 - j - 2 : j + 2].data() + column),
                    _mm256_permute2x128_si256(s2, s6, 0x20));
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(b[Mode == 3 ? w - 1 - j - 3 : j + 3].data() + column),
                    _mm256_permute2x128_si256(s3, s7, 0x20));
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(b[Mode == 3 ? w - 1 - j - 4 : j + 4].data() + column),
                    _mm256_permute2x128_si256(s0, s4, 0x31));
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(b[Mode == 3 ? w - 1 - j - 5 : j + 5].data() + column),
                    _mm256_permute2x128_si256(s1, s5, 0x31));
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(b[Mode == 3 ? w - 1 - j - 6 : j + 6].data() + column),
                    _mm256_permute2x128_si256(s2, s6, 0x31));
                _mm256_storeu_si256(reinterpret_cast<__m256i*>(b[Mode == 3 ? w - 1 - j - 7 : j + 7].data() + column),
                    _mm256_permute2x128_si256(s3, s7, 0x31));
            }
            for (; j < je; ++j) {
                const auto row = Mode == 3 ? w - 1 - j : j;
                for (std::size_t r = 0; r < 8; ++r) b[row][column + r] = source[r][j];
            }
        }
        for (; i < h; ++i) {
            const auto& row = a[i];
    #ifndef SKIP_RUNTIME_CHECKS
            if (jb == 0 && row.size() != w) throw std::invalid_argument("matrix must be rectangular");
    #endif
            for (std::size_t j = jb; j < je; ++j) {
                if constexpr (Mode == 1) b[j][h - 1 - i] = row[j];
                else if constexpr (Mode == 3) b[w - 1 - j][i] = row[j];
                else b[j][i] = row[j];
            }
        }
        jb = je;
    }
    return b;
}
}
#endif
#endif

#ifndef SIRKIT_MATRIX_TRANSFORM_DETAIL_DEFINED
#define SIRKIT_MATRIX_TRANSFORM_DETAIL_DEFINED
namespace reinsirk::detail {

// 内部用。Mode: 0=転置、1=時計回り、3=反時計回り。
template <int Mode, class Row>
std::vector<Row> matrix_turn(const std::vector<Row>& a) {
    const std::size_t h = a.size();
    if (h == 0) return {};
    const std::size_t w = a.front().size();
    if (w == 0) return {};
    using Value = typename Row::value_type;
    constexpr bool simple_value = std::is_trivially_copyable_v<Value>
        && std::is_trivially_default_constructible_v<Value>
        && std::is_trivially_copy_assignable_v<Value>;
    // 背の低い行列は出力行を順に構築し、各行の確保と書込みをまとめる。
    if constexpr (simple_value) {
        if (h < 32 && w >= 32 && !std::is_same_v<Value, bool>) {
            std::vector<Row> b;
            b.reserve(w);
            const auto append = [&]<bool Check>(std::size_t col) {
                b.emplace_back();
                auto& dest = b.back();
                dest.reserve(h);
                const std::size_t j = Mode == 3 ? w - 1 - col : col;
                for (std::size_t i = 0; i < h; ++i) {
                    const auto& row = a[Mode == 1 ? h - 1 - i : i];
#ifndef SKIP_RUNTIME_CHECKS
                    if constexpr (Check) {
                        if (row.size() != w) throw std::invalid_argument("matrix must be rectangular");
                    }
#endif
                    dest.push_back(row[j]);
                }
            };
            append.template operator()<true>(0);
            for (std::size_t j = 1; j < w; ++j) append.template operator()<false>(j);
            return b;
        }
    }
    // 大きな単純型の行列は、部分行列単位で書込み先をキャッシュに保つ。
    if constexpr (simple_value) {
        if (h >= 32 && w >= 32) {
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
            if constexpr (sizeof(Value) == 4 &&
                          (std::is_arithmetic_v<Value> || std::is_enum_v<Value>)) {
                if (h > 4095 / w && matrix_avx2_available()) {
                    if (h <= 64 && w >= 8192 && h > 524287 / w) return matrix_turn_avx2<Mode, Row, true>(a);
                    return matrix_turn_avx2<Mode>(a);
                }
            }
#endif

            std::vector<Row> b;
            b.reserve(w);
            for (std::size_t j = 0; j < w; ++j) b.emplace_back(h, Value{});
            constexpr std::size_t block = 32;
            for (std::size_t ib = 0; ib < h;) {
                const std::size_t ie = ib + std::min(block, h - ib);
                for (std::size_t jb = 0; jb < w;) {
                    const std::size_t je = jb + std::min(block, w - jb);
                    for (std::size_t i = ib; i < ie; ++i) {
                        const auto& row = a[i];
#ifndef SKIP_RUNTIME_CHECKS
                        if (jb == 0 && row.size() != w) throw std::invalid_argument("matrix must be rectangular");
#endif
                        for (std::size_t j = jb; j < je; ++j) {
                            if constexpr (Mode == 1) b[j][h - 1 - i] = row[j];
                            else if constexpr (Mode == 3) b[w - 1 - j][i] = row[j];
                            else b[j][i] = row[j];
                        }
                    }
                    jb = je;
                }
                ib = ie;
            }
            return b;
        }
    }
    // 単純型の小・縦長行列は各行を一度だけ初期化し、要素追加の分岐を省く。
    if constexpr (simple_value && !std::is_same_v<Value, bool>) {
        if (h >= 8 && (w >= 8 || h >= 32)) {
            std::vector<Row> b;
            b.reserve(w);
            for (std::size_t j = 0; j < w; ++j) b.emplace_back(h, Value{});
            for (std::size_t i = 0; i < h; ++i) {
                const auto& row = a[i];
#ifndef SKIP_RUNTIME_CHECKS
                if (row.size() != w) throw std::invalid_argument("matrix must be rectangular");
#endif
                for (std::size_t j = 0; j < w; ++j) {
                    if constexpr (Mode == 1) b[j][h - 1 - i] = row[j];
                    else if constexpr (Mode == 3) b[w - 1 - j][i] = row[j];
                    else b[j][i] = row[j];
                }
            }
            return b;
        }
    }
    std::vector<Row> b(w);
    for (auto& row : b) row.reserve(h);
    for (std::size_t i = 0; i < h; ++i) {
        const auto& row = a[Mode == 1 ? h - 1 - i : i];
#ifndef SKIP_RUNTIME_CHECKS
        if (row.size() != w) throw std::invalid_argument("matrix must be rectangular");
#endif
        for (std::size_t j = 0; j < w; ++j) {
            if constexpr (Mode == 3) b[w - 1 - j].push_back(row[j]);
            else b[j].push_back(row[j]);
        }
    }
    return b;
}

// 内部用。行のコピーと形状確認を同時に行う。
template <bool ReverseRows, bool ReverseColumns, class Row>
std::vector<Row> matrix_copy(const std::vector<Row>& a) {
    const std::size_t h = a.size();
    std::vector<Row> b;
    b.reserve(h);
    if (h == 0) return b;
#ifndef SKIP_RUNTIME_CHECKS
    const std::size_t w = a.front().size();
#endif
    for (std::size_t i = 0; i < h; ++i) {
        const auto& row = a[ReverseRows ? h - 1 - i : i];
#ifndef SKIP_RUNTIME_CHECKS
        if (row.size() != w) throw std::invalid_argument("matrix must be rectangular");
#endif
        if constexpr (ReverseColumns) b.emplace_back(row.rbegin(), row.rend());
        else b.push_back(row);
    }
    return b;
}

template <class Row>
void matrix_flip_i_inplace(std::vector<Row>& a) {
    const std::size_t h = a.size();
    if (h == 0) return;
#ifndef SKIP_RUNTIME_CHECKS
    const std::size_t w = a.front().size();
#endif
    for (std::size_t i = 0; i < h / 2; ++i) {
        auto& first = a[i];
        auto& last = a[h - 1 - i];
#ifndef SKIP_RUNTIME_CHECKS
        if (first.size() != w || last.size() != w)
            throw std::invalid_argument("matrix must be rectangular");
#endif
        first.swap(last);
    }
}

template <class Row>
void matrix_flip_j_inplace(std::vector<Row>& a) {
    if (a.empty()) return;
#ifndef SKIP_RUNTIME_CHECKS
    const std::size_t w = a.front().size();
#endif
    for (auto& row : a) {
#ifndef SKIP_RUNTIME_CHECKS
        if (row.size() != w) throw std::invalid_argument("matrix must be rectangular");
#endif
        std::reverse(row.begin(), row.end());
    }
}

template <class Row>
std::vector<Row> matrix_quarter(const std::vector<Row>& a, int turns) {
    const int r = turns % 4;
    switch (r < 0 ? r + 4 : r) {
        case 0: return matrix_copy<false, false>(a);
        case 1: return matrix_turn<1>(a);
        case 2: return matrix_copy<true, true>(a);
        default: return matrix_turn<3>(a);
    }
}
} // namespace reinsirk::detail
#endif

#ifndef SIRKIT_ROTATE_90R_DEFINED
#define SIRKIT_ROTATE_90R_DEFINED
template <class T>
std::vector<std::vector<T>> rotate_90r(const std::vector<std::vector<T>>& a) {
    if (a.empty()) return std::vector<std::vector<T>>(1);
    return reinsirk::detail::matrix_turn<1>(a);
}
inline std::vector<std::string> rotate_90r(const std::vector<std::string>& a) {
    if (a.empty()) return std::vector<std::string>(1);
    return reinsirk::detail::matrix_turn<1>(a);
}
#endif

#ifndef SIRKIT_FLIP_J_DEFINED
#define SIRKIT_FLIP_J_DEFINED
template <class T>
std::vector<std::vector<T>> flip_j(const std::vector<std::vector<T>>& a) {
    if (a.empty()) return std::vector<std::vector<T>>(1);
    return reinsirk::detail::matrix_copy<false, true>(a);
}
inline std::vector<std::string> flip_j(const std::vector<std::string>& a) {
    if (a.empty()) return std::vector<std::string>(1);
    return reinsirk::detail::matrix_copy<false, true>(a);
}
#endif

#ifndef SIRKIT_FLIP_I_DEFINED
#define SIRKIT_FLIP_I_DEFINED
template <class T>
std::vector<std::vector<T>> flip_i(const std::vector<std::vector<T>>& a) {
    if (a.empty()) return std::vector<std::vector<T>>(1);
    return reinsirk::detail::matrix_copy<true, false>(a);
}
inline std::vector<std::string> flip_i(const std::vector<std::string>& a) {
    if (a.empty()) return std::vector<std::string>(1);
    return reinsirk::detail::matrix_copy<true, false>(a);
}
#endif

#ifndef SIRKIT_ROTATE_90L_DEFINED
#define SIRKIT_ROTATE_90L_DEFINED
template <class T>
std::vector<std::vector<T>> rotate_90l(const std::vector<std::vector<T>>& a) {
    return reinsirk::detail::matrix_turn<3>(a);
}
inline std::vector<std::string> rotate_90l(const std::vector<std::string>& a) {
    return reinsirk::detail::matrix_turn<3>(a);
}
#endif

#ifndef SIRKIT_ROTATE_180_DEFINED
#define SIRKIT_ROTATE_180_DEFINED
template <class T>
std::vector<std::vector<T>> rotate_180(const std::vector<std::vector<T>>& a) {
    return reinsirk::detail::matrix_copy<true, true>(a);
}
inline std::vector<std::string> rotate_180(const std::vector<std::string>& a) {
    return reinsirk::detail::matrix_copy<true, true>(a);
}
#endif

#ifndef SIRKIT_ROTATE_QUARTER_DEFINED
#define SIRKIT_ROTATE_QUARTER_DEFINED
template <class T>
std::vector<std::vector<T>> rotate_quarter(const std::vector<std::vector<T>>& a, int clockwise_turns) {
    return reinsirk::detail::matrix_quarter(a, clockwise_turns);
}
inline std::vector<std::string> rotate_quarter(const std::vector<std::string>& a, int clockwise_turns) {
    return reinsirk::detail::matrix_quarter(a, clockwise_turns);
}
#endif

#ifndef SIRKIT_TRANSPOSE_DEFINED
#define SIRKIT_TRANSPOSE_DEFINED
template <class T>
std::vector<std::vector<T>> transpose(const std::vector<std::vector<T>>& a) {
    return reinsirk::detail::matrix_turn<0>(a);
}
inline std::vector<std::string> transpose(const std::vector<std::string>& a) {
    return reinsirk::detail::matrix_turn<0>(a);
}
#endif

#ifndef SIRKIT_FLIP_I_INPLACE_DEFINED
#define SIRKIT_FLIP_I_INPLACE_DEFINED
template <class T>
void flip_i_inplace(std::vector<std::vector<T>>& a) {
    reinsirk::detail::matrix_flip_i_inplace(a);
}
inline void flip_i_inplace(std::vector<std::string>& a) {
    reinsirk::detail::matrix_flip_i_inplace(a);
}
#endif

#ifndef SIRKIT_FLIP_J_INPLACE_DEFINED
#define SIRKIT_FLIP_J_INPLACE_DEFINED
template <class T>
void flip_j_inplace(std::vector<std::vector<T>>& a) {
    reinsirk::detail::matrix_flip_j_inplace(a);
}
inline void flip_j_inplace(std::vector<std::string>& a) {
    reinsirk::detail::matrix_flip_j_inplace(a);
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

#include <vector>
int main(){
    int n{};verify_in>>n;std::vector<std::vector<int>>a(n,std::vector<int>(n)),b=a;
    for(auto&r:a)for(auto&x:r)verify_in>>x;
    for(auto&r:b)for(auto&x:r)verify_in>>x;
    for(int turns=0;turns<4;++turns){bool ok=true;for(int i=0;i<n;++i)for(int j=0;j<n;++j)if(a[i][j]&&!b[i][j])ok=false;
        if(ok){verify_out<<"Yes\n";return 0;}a=rotate_90r(a);
    }verify_out<<"No\n";
}
