#define SKIP_RUNTIME_CHECKS
// Verify: https://judge.yosupo.jp/problem/bitwise_and_convolution

// Library: Math/Zeta/Bitwise_Convolution.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/set-function/zeta-mobius-transform.hpp.html
#include <bit>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

// 集合変換は空配列または長さ2^k。各演算はO(n log n)、追加領域O(1)。
// superset_zeta_transform(f): f[S]をSの上位集合の総和へ更新する。
// superset_mobius_transform(f): 上位集合ゼータ変換を元に戻す。
// subset_zeta_transform(f): f[S]をSの部分集合の総和へ更新する。
// subset_mobius_transform(f): 部分集合ゼータ変換を元に戻す。
#ifndef SIRKIT_SET_TRANSFORM_SIMD_DETAIL_DEFINED
#define SIRKIT_SET_TRANSFORM_SIMD_DETAIL_DEFINED
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
namespace reinsirk::detail {
template <bool Subset, bool Subtract, std::size_t Half, class T>
[[gnu::always_inline]] inline void set_transform_small_stage(T* f, std::size_t n) {
    for (std::size_t base=0; base<n; base+=Half*2) {
        T* out=f+base+(Subset?Half:0);
        const T* in=f+base+(Subset?0:Half);
        for (std::size_t j=0;j<Half;++j) {
            if constexpr(Subtract) out[j]-=in[j];else out[j]+=in[j];
        }
    }
}
template <bool Subset, bool Subtract, class T>
[[gnu::always_inline]] inline void set_transform_small(T* f,std::size_t n) {
    if(n>=2)set_transform_small_stage<Subset,Subtract,1>(f,n);
    if(n>=4)set_transform_small_stage<Subset,Subtract,2>(f,n);
    if(n>=8)set_transform_small_stage<Subset,Subtract,4>(f,n);
    if(n>=16)set_transform_small_stage<Subset,Subtract,8>(f,n);
    if(n>=32)set_transform_small_stage<Subset,Subtract,16>(f,n);
}
template <bool Subset, bool Subtract, unsigned Bit, class U, class V, std::size_t... I>
[[gnu::target("avx2"),gnu::always_inline]] inline V set_transform_stage(V v, std::index_sequence<I...>) {
    const V swapped = __builtin_shufflevector(v, v, (I ^ Bit)...);
    const V mask{static_cast<U>((static_cast<bool>(I & Bit) == Subset) ? static_cast<U>(~U{0}) : U{0})...};
    if constexpr (Subtract) v -= swapped & mask;
    else v += swapped & mask;
    if constexpr (Bit * 2 < sizeof(V) / sizeof(U))
        return set_transform_stage<Subset,Subtract,Bit*2,U>(v, std::index_sequence<I...>{});
    return v;
}
template <bool Subset, bool Subtract, class T>
[[gnu::target("avx2")]] void set_transform_simd(T* f, std::size_t n) {
    using U = std::make_unsigned_t<T>;
    typedef U V __attribute__((vector_size(32)));
    constexpr std::size_t width = sizeof(V) / sizeof(T);
    for (std::size_t base = 0; base < n; base += width) {
        V v;
        std::memcpy(&v, f + base, sizeof(v));
        v = set_transform_stage<Subset,Subtract,1,U>(v, std::make_index_sequence<width>{});
        std::memcpy(f + base, &v, sizeof(v));
    }
    for (std::size_t half = width; half < n; half *= 2) {
        for (std::size_t base = 0; base < n; base += half * 2) {
            T* out = f + base + (Subset ? half : 0);
            const T* in = f + base + (Subset ? 0 : half);
            for (std::size_t j = 0; j < half; j += width) {
                V a, b;
                std::memcpy(&a, out + j, sizeof(a));
                std::memcpy(&b, in + j, sizeof(b));
                if constexpr (Subtract) a -= b;
                else a += b;
                std::memcpy(out + j, &a, sizeof(a));
            }
        }
    }
}
}
#endif
#endif


#ifndef SIRKIT_SUPERSET_ZETA_TRANSFORM_DEFINED
#define SIRKIT_SUPERSET_ZETA_TRANSFORM_DEFINED
template <class T> void superset_zeta_transform(std::vector<T>& f) {
#ifndef SKIP_RUNTIME_CHECKS
    if (!f.empty() && !std::has_single_bit(f.size()))
        throw std::invalid_argument("superset_zeta_transform: length must be a power of two");
#endif
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T,bool> && sizeof(T)<=8) {
        if (f.size()>=8) {
        if(f.size()==8){f[0]+=f[1];f[2]+=f[3];f[4]+=f[5];f[6]+=f[7];f[0]+=f[2];f[1]+=f[3];f[4]+=f[6];f[5]+=f[7];f[0]+=f[4];f[1]+=f[5];f[2]+=f[6];f[3]+=f[7];return;}
        if (f.size()<64) {
            switch(f.size()) {
            case 16:reinsirk::detail::set_transform_small<false,false>(f.data(),16);return;
            case 32:reinsirk::detail::set_transform_small<false,false>(f.data(),32);return;
            default:return;
            }
        }
        if (__builtin_cpu_supports("avx2")) {
            reinsirk::detail::set_transform_simd<false,false>(f.data(), f.size());
            return;
        }
        }
    }
#endif
    for (std::size_t half = 1; half < f.size(); half *= 2) {
        for (std::size_t base = 0; base < f.size(); base += half * 2) {
            for (std::size_t j = 0; j < half; ++j) f[base + j] += f[base + half + j];
        }
    }
}
#endif

#ifndef SIRKIT_SUPERSET_MOBIUS_TRANSFORM_DEFINED
#define SIRKIT_SUPERSET_MOBIUS_TRANSFORM_DEFINED
template <class T> void superset_mobius_transform(std::vector<T>& f) {
#ifndef SKIP_RUNTIME_CHECKS
    if (!f.empty() && !std::has_single_bit(f.size()))
        throw std::invalid_argument("superset_mobius_transform: length must be a power of two");
#endif
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T,bool> && sizeof(T)<=8) {
        if (f.size()>=8) {
        if(f.size()==8){f[0]-=f[1];f[2]-=f[3];f[4]-=f[5];f[6]-=f[7];f[0]-=f[2];f[1]-=f[3];f[4]-=f[6];f[5]-=f[7];f[0]-=f[4];f[1]-=f[5];f[2]-=f[6];f[3]-=f[7];return;}
        if (f.size()<64) {
            switch(f.size()) {
            case 16:reinsirk::detail::set_transform_small<false,true>(f.data(),16);return;
            case 32:reinsirk::detail::set_transform_small<false,true>(f.data(),32);return;
            default:return;
            }
        }
        if (__builtin_cpu_supports("avx2")) {
            reinsirk::detail::set_transform_simd<false,true>(f.data(), f.size());
            return;
        }
        }
    }
#endif
    for (std::size_t half = 1; half < f.size(); half *= 2) {
        for (std::size_t base = 0; base < f.size(); base += half * 2) {
            for (std::size_t j = 0; j < half; ++j) f[base + j] -= f[base + half + j];
        }
    }
}
#endif

#ifndef SIRKIT_SUBSET_ZETA_TRANSFORM_DEFINED
#define SIRKIT_SUBSET_ZETA_TRANSFORM_DEFINED
template <class T> void subset_zeta_transform(std::vector<T>& f) {
#ifndef SKIP_RUNTIME_CHECKS
    if (!f.empty() && !std::has_single_bit(f.size()))
        throw std::invalid_argument("subset_zeta_transform: length must be a power of two");
#endif
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T,bool> && sizeof(T)<=8) {
        if (f.size()>=8) {
        if(f.size()==8){f[1]+=f[0];f[3]+=f[2];f[5]+=f[4];f[7]+=f[6];f[2]+=f[0];f[3]+=f[1];f[6]+=f[4];f[7]+=f[5];f[4]+=f[0];f[5]+=f[1];f[6]+=f[2];f[7]+=f[3];return;}
        if (f.size()<64) {
            switch(f.size()) {
            case 16:reinsirk::detail::set_transform_small<true,false>(f.data(),16);return;
            case 32:reinsirk::detail::set_transform_small<true,false>(f.data(),32);return;
            default:return;
            }
        }
        if (__builtin_cpu_supports("avx2")) {
            reinsirk::detail::set_transform_simd<true,false>(f.data(), f.size());
            return;
        }
        }
    }
#endif
    for (std::size_t half = 1; half < f.size(); half *= 2) {
        for (std::size_t base = 0; base < f.size(); base += half * 2) {
            for (std::size_t j = 0; j < half; ++j) f[base + half + j] += f[base + j];
        }
    }
}
#endif

#ifndef SIRKIT_SUBSET_MOBIUS_TRANSFORM_DEFINED
#define SIRKIT_SUBSET_MOBIUS_TRANSFORM_DEFINED
template <class T> void subset_mobius_transform(std::vector<T>& f) {
#ifndef SKIP_RUNTIME_CHECKS
    if (!f.empty() && !std::has_single_bit(f.size()))
        throw std::invalid_argument("subset_mobius_transform: length must be a power of two");
#endif
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T,bool> && sizeof(T)<=8) {
        if (f.size()>=8) {
        if(f.size()==8){f[1]-=f[0];f[3]-=f[2];f[5]-=f[4];f[7]-=f[6];f[2]-=f[0];f[3]-=f[1];f[6]-=f[4];f[7]-=f[5];f[4]-=f[0];f[5]-=f[1];f[6]-=f[2];f[7]-=f[3];return;}
        if (f.size()<64) {
            switch(f.size()) {
            case 16:reinsirk::detail::set_transform_small<true,true>(f.data(),16);return;
            case 32:reinsirk::detail::set_transform_small<true,true>(f.data(),32);return;
            default:return;
            }
        }
        if (__builtin_cpu_supports("avx2")) {
            reinsirk::detail::set_transform_simd<true,true>(f.data(), f.size());
            return;
        }
        }
    }
#endif
    for (std::size_t half = 1; half < f.size(); half *= 2) {
        for (std::size_t base = 0; base < f.size(); base += half * 2) {
            for (std::size_t j = 0; j < half; ++j) f[base + half + j] -= f[base + j];
        }
    }
}
#endif

// bitwise_and_convolution(f,g): h[k]=sum_{i&j=k}f[i]g[j]。同じ長さ2^k、O(n log n)。
// bitwise_or_convolution(f,g): h[k]=sum_{i|j=k}f[i]g[j]。同じ長さ2^k、O(n log n)。
#ifndef SIRKIT_BITWISE_AND_CONVOLUTION_DEFINED
#define SIRKIT_BITWISE_AND_CONVOLUTION_DEFINED
template <class T> std::vector<T> bitwise_and_convolution(std::vector<T> f, std::vector<T> g) {
#ifndef SKIP_RUNTIME_CHECKS
    if (f.size() != g.size()) throw std::invalid_argument("bitwise_and_convolution: different lengths");
#endif
    superset_zeta_transform(f);
    superset_zeta_transform(g);
    for (std::size_t i = 0; i < f.size(); ++i) f[i] *= g[i];
    superset_mobius_transform(f);
    return f;
}
#endif

#ifndef SIRKIT_BITWISE_OR_CONVOLUTION_DEFINED
#define SIRKIT_BITWISE_OR_CONVOLUTION_DEFINED
template <class T> std::vector<T> bitwise_or_convolution(std::vector<T> f, std::vector<T> g) {
#ifndef SKIP_RUNTIME_CHECKS
    if (f.size() != g.size()) throw std::invalid_argument("bitwise_or_convolution: different lengths");
#endif
    subset_zeta_transform(f);
    subset_zeta_transform(g);
    for (std::size_t i = 0; i < f.size(); ++i) f[i] *= g[i];
    subset_mobius_transform(f);
    return f;
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
#include <cassert>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

// 提出先のACL有無に依存しない、このドライバ専用の法998244353整数。
struct VerifyMod998 {
    std::uint32_t value = 0;
    VerifyMod998() = default;
    VerifyMod998(std::int64_t x) {
        x %= 998244353;
        if (x < 0) x += 998244353;
        value = static_cast<std::uint32_t>(x);
    }
    VerifyMod998& operator+=(VerifyMod998 b) { value += b.value; if (value >= 998244353) value -= 998244353; return *this; }
    VerifyMod998& operator-=(VerifyMod998 b) { if (value < b.value) value += 998244353; value -= b.value; return *this; }
    VerifyMod998& operator*=(VerifyMod998 b) { value = static_cast<std::uint32_t>(std::uint64_t(value) * b.value % 998244353); return *this; }
    friend VerifyMod998 operator+(VerifyMod998 a, VerifyMod998 b) { return a += b; }
    friend VerifyMod998 operator-(VerifyMod998 a, VerifyMod998 b) { return a -= b; }
    friend VerifyMod998 operator*(VerifyMod998 a, VerifyMod998 b) { return a *= b; }
    friend bool operator==(VerifyMod998, VerifyMod998) = default;
};

int main() {
    int exponent{}; verify_in >> exponent;
    const std::size_t n = std::size_t{1} << exponent;
    std::vector<VerifyMod998> a(n), b(n);
    for (auto& x : a) { std::int64_t v{}; verify_in >> v; x = v; }
    for (auto& x : b) { std::int64_t v{}; verify_in >> v; x = v; }
    std::reverse(a.begin(), a.end()); std::reverse(b.begin(), b.end());
    auto c = bitwise_or_convolution(a, b);
    std::reverse(c.begin(), c.end());
    for (std::size_t i = 0; i < n; ++i) verify_out << c[i].value << (i + 1 == n ? '\n' : ' ');
}
