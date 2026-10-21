#define SKIP_RUNTIME_CHECKS
// Verify: https://judge.yosupo.jp/problem/primality_test

// Library: Math/prime_judge(Miller-Rabin).cpp
// SPDX-License-Identifier: CC0-1.0

#include <bit>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <type_traits>

#ifndef REINSIRK_PRIMALITY32_DEFINED
#define REINSIRK_PRIMALITY32_DEFINED
namespace reinsirk::detail {
// uint32_t全域の素数判定。Miller-Rabinの底2,7,61を使う。
// 理論参考: G. Jaeschke (1993), https://doi.org/10.1090/S0025-5718-1993-1192971-8
inline bool is_prime_u32(std::uint32_t n) noexcept
{
    // 競技プログラミングでよく使う素数は、累乗計算を省く。
    switch (n)
    {
    case 167772161u:
    case 469762049u:
    case 754974721u:
    case 998244353u:
    case 1000000007u:
    case 1000000009u:
    case 1004535809u:
    case 1224736769u:
    case 2013265921u:
        return true;
    default:
        break;
    }
    if (n < 2)
        return false;
    for (std::uint32_t p : {2u, 3u, 5u, 7u, 11u})
    {
        if (n % p == 0)
            return n == p;
    }
    std::uint32_t odd = n - 1;
    unsigned shifts = 0;
    while ((odd & 1u) == 0)
    {
        odd >>= 1;
        ++shifts;
    }
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
    // R=2^64。n<2^32なので、累乗中の剰余は2n未満のまま保持できる。
    using u64 = std::uint64_t;
    using u128 = __uint128_t; // GCC/Clangの128ビット整数拡張。
    u64 inverse = n;
    for (int i = 0; i < 5; ++i) inverse *= 2u - u64(n) * inverse;
    const u64 negative_inverse = u64{0} - inverse;
    const auto multiply = [n, negative_inverse](u64 a, u64 b) {
        const u128 product = static_cast<u128>(a) * b;
        const u128 correction = static_cast<u128>(static_cast<u64>(product) * negative_inverse) * n;
        return static_cast<u64>((product + correction) >> 64);
    };
    const u64 one = (u64{0} - n) % n;
    const u64 r_squared = one * one % n;
    for (std::uint32_t base : {2u, 7u, 61u}) {
        if (base % n == 0) continue;
        u64 value = one, power = multiply(base % n, r_squared);
        for (std::uint32_t exponent = odd; exponent; exponent >>= 1) {
            if (exponent & 1u) value = multiply(value, power);
            power = multiply(power, power);
        }
        if (value >= n) value -= n;
        if (value == one || value == n - one) continue;
        unsigned step = 1;
        for (; step < shifts; ++step) {
            value = multiply(value, value);
            if (value >= n) value -= n;
            if (value == n - one) break;
        }
        if (step == shifts) return false;
    }
#else
    // R=2^32のMontgomery乗算。33ビットの還元結果もuint64_tで扱う。
    std::uint32_t inverse = n;
    for (int i = 0; i < 5; ++i)
        inverse *= 2u - n * inverse;
    const std::uint32_t negative_inverse = 0u - inverse;
    const auto multiply = [n, negative_inverse](std::uint32_t a, std::uint32_t b)
    {
        const std::uint64_t product = static_cast<std::uint64_t>(a) * b;
        const std::uint32_t factor = static_cast<std::uint32_t>(product) * negative_inverse;
        const std::uint64_t correction = static_cast<std::uint64_t>(factor) * n;
        std::uint64_t result = (product >> 32) + (correction >> 32) +
                               (static_cast<std::uint32_t>(product) != 0);
        if (result >= n)
            result -= n;
        return static_cast<std::uint32_t>(result);
    };
    const auto one = static_cast<std::uint32_t>((std::uint64_t{1} << 32) % n);
    const auto r_squared = static_cast<std::uint32_t>(static_cast<std::uint64_t>(one) * one % n);
    for (std::uint32_t base : {2u, 7u, 61u})
    {
        if (base % n == 0)
            continue;
        std::uint32_t value = one, power = multiply(base % n, r_squared);
        for (std::uint32_t exponent = odd; ;)
        {
            if (exponent & 1u)
                value = multiply(value, power);
            exponent >>= 1;
            if (exponent == 0) break;
            power = multiply(power, power);
        }
        if (value == one || value == n - one)
            continue;
        unsigned step = 1;
        for (; step < shifts; ++step)
        {
            value = multiply(value, value);
            if (value == n - one)
                break;
        }
        if (step == shifts)
            return false;
    }
#endif
    return true;
}
} // namespace reinsirk::detail
#endif

// C++20以降。64ビット整数用の内部演算。状態は判定ごとに持つ。
#ifndef REINSIRK_MILLER_RABIN64_DEFINED
#define REINSIRK_MILLER_RABIN64_DEFINED
namespace reinsirk::detail {
template <bool Relaxed>
class MillerRabin64
{
    using u64 = std::uint64_t;
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
    using u128 = __uint128_t; // GCC/Clangの拡張。未対応環境では下の標準実装を使う。
#else
    struct Wide
    {
        u64 low, high;
    };
#endif
    const u64 modulus_;
    u64 negative_inverse_, one_, r_squared_;
    u64 odd_;
    unsigned shifts_ = 0;

#if !((defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__))
    // 128ビット整数のない環境では32ビットずつに分けて積を得る。
    static Wide product(u64 a, u64 b) noexcept
    {
        const u64 a0 = static_cast<std::uint32_t>(a), a1 = a >> 32;
        const u64 b0 = static_cast<std::uint32_t>(b), b1 = b >> 32;
        const u64 low = a0 * b0;
        const u64 middle = a1 * b0 + (low >> 32);
        const u64 upper_middle = a0 * b1 + static_cast<std::uint32_t>(middle);
        return {(upper_middle << 32) | static_cast<std::uint32_t>(low),
                a1 * b1 + (middle >> 32) + (upper_middle >> 32)};
    }
#endif

    // 通常はa,b<modulus_。Relaxedではmodulus_<2^62、a,b<2*modulus_。
    u64 multiply(u64 a, u64 b) const noexcept
    {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
        const u128 value = static_cast<u128>(a) * b;
        const u128 correction = static_cast<u128>(static_cast<u64>(value) * negative_inverse_) * modulus_;
        const u128 sum = value + correction;
        const u64 result = static_cast<u64>(sum >> 64);
        if constexpr (Relaxed)
            return result;
        else
        {
            if (sum < value || result >= modulus_)
                return result - modulus_;
            return result;
        }
#else
        const auto value = product(a, b);
        const auto correction = product(value.low * negative_inverse_, modulus_);
        const u64 upper = value.high + correction.high;
        const u64 result = upper + (value.low != 0);
        if constexpr (Relaxed)
            return result; // 2*modulus_未満のまま保持し、累乗中の条件分岐を省く。
        else
        {
            // 129ビット目の桁上がりがある場合も、減算後はmodulus_未満。
            if (upper < value.high || result < upper || result >= modulus_)
                return result - modulus_;
            return result;
        }
#endif
    }

    u64 power(u64 base, u64 exponent) const noexcept
    {
        u64 result = one_;
        while (exponent)
        {
            if (exponent & 1u)
                result = multiply(result, base);
            base = multiply(base, base);
            exponent >>= 1;
        }
        return result;
    }

    u64 normalize(u64 value) const noexcept
    {
        if constexpr (Relaxed)
            return value >= modulus_ ? value - modulus_ : value;
        else
            return value;
    }

public:
    // 内部専用。奇数n>=3についてMontgomery定数とn-1の分解を準備する。
    explicit MillerRabin64(u64 n) noexcept : modulus_(n), odd_(n - 1)
    {
        u64 inverse = n;
        for (int i = 0; i < 5; ++i)
            inverse *= 2u - n * inverse;
        negative_inverse_ = 0u - inverse;
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)
        r_squared_ = (u128{0} - n) % n; // 2^128 mod n。
        one_ = multiply(1, r_squared_);
#else
        one_ = (u64{0} - n) % n; // 2^64 mod n。
        // 16Rを4回平方するとR^2になる（R=2^64）。
        r_squared_ = one_;
        for (int i = 0; i < 4; ++i)
        {
            const u64 gap = n - r_squared_;
            r_squared_ = r_squared_ >= gap ? r_squared_ - gap : r_squared_ + r_squared_;
        }
        for (int i = 0; i < 4; ++i)
            r_squared_ = multiply(r_squared_, r_squared_);
#endif
        shifts_ = std::countr_zero(odd_);
        odd_ >>= shifts_;
    }

    // 指定した底の強擬素数検査を全て通ればtrue。底は整数型とする。
    template <class Bases>
    bool test(const Bases &bases) const noexcept
    {
        for (auto base : bases)
        {
            using Base = decltype(base);
            static_assert(std::is_integral_v<Base> && sizeof(Base) <= sizeof(u64),
                          "MillerRabin: bases must be integers of at most 64 bits");
            u64 reduced;
            if constexpr (std::is_signed_v<Base>)
            {
                if (base < 0)
                {
                    const u64 magnitude = static_cast<u64>(-(base + 1)) + 1;
                    const u64 remainder = magnitude % modulus_;
                    reduced = remainder ? modulus_ - remainder : 0;
                }
                else
                    reduced = static_cast<u64>(base) % modulus_;
            }
            else
                reduced = static_cast<u64>(base) % modulus_;
            if (reduced == 0)
                continue;
            u64 value = normalize(power(multiply(reduced, r_squared_), odd_));
            if (value == one_ || value == modulus_ - one_)
                continue;
            unsigned step = 1;
            for (; step < shifts_; ++step)
            {
                value = normalize(multiply(value, value));
                if (value == modulus_ - one_)
                    break;
            }
            if (step == shifts_)
                return false;
        }
        return true;
    }
};
} // namespace reinsirk::detail
#endif

#ifndef REINSIRK_MILLER_RABIN_DEFINED
#define REINSIRK_MILLER_RABIN_DEFINED
// MillerRabin(n, bases): 指定した底の強擬素数検査を通ればtrue。底の選択により合成数も通る。
// n<2はfalse。底はnで剰余を取り、0なら省く。奇数n>=3で空の底列ならtrue。
template <class Integer, class Bases>
inline bool MillerRabin(Integer n, const Bases &bases) noexcept
{
    static_assert(std::is_integral_v<Integer> && sizeof(Integer) <= sizeof(std::uint64_t),
                  "MillerRabin: n must be an integer of at most 64 bits");
    if constexpr (std::is_signed_v<Integer>)
        if (n < 0)
            return false;
    const auto value = static_cast<std::uint64_t>(n);
    if (value < 2)
        return false;
    if ((value & 1u) == 0)
        return value == 2;
    if (value < (std::uint64_t{1} << 62))
        return reinsirk::detail::MillerRabin64<true>(value).test(bases);
    return reinsirk::detail::MillerRabin64<false>(value).test(bases);
}

// MillerRabin(n, {2, 7, 61})のような底の直接指定に対応する。
template <class Integer>
inline bool MillerRabin(Integer n, std::initializer_list<std::uint64_t> bases) noexcept
{
    return MillerRabin<Integer, std::initializer_list<std::uint64_t>>(n, bases);
}
#endif

#ifndef REINSIRK_IS_PRIME_DEFINED
#define REINSIRK_IS_PRIME_DEFINED
// is_prime(n): 符号付きの負数はfalse、非負整数はuint64_t全域で素数かを確定する。
// 32ビット以下は共通判定、4'759'123'141未満は3個、それ以上は7個の底。動的確保なし。
// 底の理論参考: Jim Sinclair, https://miller-rabin.appspot.com/
template <class Integer>
inline bool is_prime(Integer n) noexcept
{
    static_assert(std::is_integral_v<Integer> && sizeof(Integer) <= sizeof(std::uint64_t),
                  "is_prime: n must be an integer of at most 64 bits");
    if constexpr (std::is_signed_v<Integer>)
        if (n < 0)
            return false;
    const auto value = static_cast<std::uint64_t>(n);
    if (value < 2)
        return false;
    if (value <= std::numeric_limits<std::uint32_t>::max())
        return reinsirk::detail::is_prime_u32(static_cast<std::uint32_t>(value));
    for (std::uint32_t prime : {2u, 3u, 5u, 7u, 11u})
        if (value % prime == 0)
            return false;
    if (value < std::uint64_t{4'759'123'141})
        return MillerRabin(value, {2, 7, 61});
    return MillerRabin(value, {2, 325, 9375, 28178, 450775, 9780504, 1795265022});
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

int main() {
    int q{};verify_in>>q;while(q--) { std::uint64_t n{};verify_in>>n;verify_out<<(is_prime(n)?"Yes":"No")<<'\n'; }
}
