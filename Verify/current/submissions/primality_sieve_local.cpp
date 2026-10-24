// Verify: ローカル独立検査

// Library: Math/prime_judge(Miller-Rabin).cpp
// SPDX-License-Identifier: CC0-1.0
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/primality.md
//
// API一覧
// [MillerRabin / is_prime / 隣接素数]
// MillerRabin(n, bases): 指定した底の強擬素数検査を通ればtrue。底の選択により合成数も通る。
// n<2はfalse。底はnで剰余を取り、0なら省く。奇数n>=3で空の底列ならtrue。
// k個の底にO(k log n)回の剰余乗算、追加領域O(1)。独自の底列の例外は伝える。
//
// MillerRabin(n, {2, 7, 61})のような底の直接指定に対応する。
//
// is_prime(n): 符号付きの負数はfalse、非負整数はuint64_t全域で素数かを確定する。
// 32ビット以下は共通判定、4'759'123'141未満は3個、それ以上は7個の底。O(log n)回の剰余乗算、動的確保なし。
//
// next_prime(n): n以上で最小の素数をoptional<uint64_t>で返す。負数は2、上端で存在しなければnullopt。
// prev_prime(n): n以下で最大の素数。同じ戻り値型で、n<2はnullopt。入力は64bit以下の整数。
// C候補の探索にO(C log(max(n,2)))回の剰余乗算、追加領域O(1)。動的確保なし。

#include <bit>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <optional>
#include <utility>
#include <type_traits>

#ifndef SIRKIT_PRIMALITY32_DEFINED
#define SIRKIT_PRIMALITY32_DEFINED
namespace reinsirk::detail {
// uint32_t全域の素数判定。Miller-Rabinの底2,7,61を使う。
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
    if (n % 2u == 0) return n == 2u;
    if (n % 3u == 0) return n == 3u;
    if (n % 5u == 0) return n == 5u;
    if (n % 7u == 0) return n == 7u;
    if (n % 11u == 0) return n == 11u;
    std::uint32_t odd = n - 1;
    unsigned shifts = 0;
    while ((odd & 1u) == 0)
    {
        odd >>= 1;
        ++shifts;
    }
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
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
#ifndef SIRKIT_MILLER_RABIN64_DEFINED
#define SIRKIT_MILLER_RABIN64_DEFINED
namespace reinsirk::detail {
// member begin/endと配列だけを判定し、ADL専用の列は保守的にfalseとする。
template<class Bases>
inline constexpr bool miller_rabin_bases_nothrow = [] {
    if constexpr (requires(const Bases& bases) { bases.begin(); bases.end(); }) {
        using Begin = decltype(std::declval<const Bases&>().begin());
        using End = decltype(std::declval<const Bases&>().end());
        using Iterator = std::decay_t<Begin>;
        using Sentinel = std::decay_t<End>;
        if constexpr (requires(Iterator& it, Sentinel& end) { ++it; *it; static_cast<bool>(it != end); }) {
            return noexcept(std::declval<const Bases&>().begin()) &&
                   noexcept(std::declval<const Bases&>().end()) &&
                   std::is_nothrow_constructible_v<Iterator, Begin> &&
                   std::is_nothrow_constructible_v<Sentinel, End> &&
                   noexcept(++std::declval<Iterator&>()) &&
                   noexcept(*std::declval<Iterator&>()) &&
                   noexcept(static_cast<bool>(std::declval<Iterator&>() != std::declval<Sentinel&>())) &&
                   std::is_nothrow_destructible_v<Iterator> && std::is_nothrow_destructible_v<Sentinel>;
        } else return false;
    } else return std::is_array_v<Bases>;
}();

template <bool Relaxed>
class MillerRabin64
{
    using u64 = std::uint64_t;
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
    using u128 = __uint128_t; // GCC/Clangの拡張。未対応環境では下の標準実装を使う。
#else
    struct Wide
    {
        u64 low, high;
    };
#endif
    const u64 modulus_;
    u64 reduction_inverse_, one_, r_squared_;
    u64 odd_;
    unsigned shifts_ = 0;

#if !((defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC))
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
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
        const u128 value = static_cast<u128>(a) * b;
        const u64 low = static_cast<u64>(value);
        const u64 high = static_cast<u64>(value >> 64);
        const u64 correction_high = static_cast<u64>((static_cast<u128>(low * reduction_inverse_) * modulus_) >> 64);
#else
        const auto value = product(a, b);
        const u64 high = value.high;
        const u64 correction_high = product(value.low * reduction_inverse_, modulus_).high;
#endif
        if constexpr (Relaxed)
            return high + modulus_ - correction_high;
        else {
            const u64 result = high - correction_high;
            return high < correction_high ? result + modulus_ : result;
        }
    }

    u64 power(u64 base, u64 exponent) const noexcept {
        u64 result = one_;
        for (;;) {
            if (exponent & 1u) result = multiply(result, base);
            exponent >>= 1;
            if (!exponent) return result;
            base = multiply(base, base);
        }
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
        reduction_inverse_ = inverse;
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
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
    bool test(const Bases &bases) const noexcept(miller_rabin_bases_nothrow<Bases>)
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
    // 固定7底用。n>=4'759'123'141で、底2の通過後に残りを3個ずつ進める。
    bool test_deterministic() const noexcept {
        if (!test(std::initializer_list<u64>{2})) return false;
        return test_triple(325, 9375, 28178) && test_triple(450775, 9780504, 1795265022);
    }
private:
    bool test_triple(u64 a, u64 b, u64 c) const noexcept {
        u64 ta[4], tb[4], tc[4];
        ta[0] = tb[0] = tc[0] = one_;
        ta[1] = multiply(a, r_squared_); tb[1] = multiply(b, r_squared_); tc[1] = multiply(c, r_squared_);
        for (unsigned i = 2; i < 4; ++i) {
            ta[i] = multiply(ta[i-1], ta[1]); tb[i] = multiply(tb[i-1], tb[1]); tc[i] = multiply(tc[i-1], tc[1]);
        }
        const unsigned width = std::bit_width(odd_);
        const unsigned first_bits = (width - 1) % 2 + 1;
        int shift = static_cast<int>(width - first_bits);
        unsigned chunk = static_cast<unsigned>(odd_ >> shift);
        u64 x = ta[chunk], y = tb[chunk], z = tc[chunk];
        for (shift -= 2; shift >= 0; shift -= 2) {
            x = multiply(x,x); y = multiply(y,y); z = multiply(z,z);
            x = multiply(x,x); y = multiply(y,y); z = multiply(z,z);
            chunk = static_cast<unsigned>((odd_ >> shift) & 3u);
            if (chunk != 0) {
                x = multiply(x,ta[chunk]); y = multiply(y,tb[chunk]); z = multiply(z,tc[chunk]);
            }
        }
        x = normalize(x); y = normalize(y); z = normalize(z);
        bool pass_a = x == one_ || x == modulus_ - one_;
        bool pass_b = y == one_ || y == modulus_ - one_;
        bool pass_c = z == one_ || z == modulus_ - one_;
        // 一度通った底の結果は、ほかの底の平方を続けても保持する。
        for (unsigned step = 1; step < shifts_ && !(pass_a && pass_b && pass_c); ++step) {
            x = normalize(multiply(x, x)); y = normalize(multiply(y, y)); z = normalize(multiply(z, z));
            pass_a |= x == modulus_ - one_;
            pass_b |= y == modulus_ - one_;
            pass_c |= z == modulus_ - one_;
        }
        return pass_a && pass_b && pass_c;
    }

};
} // namespace reinsirk::detail
#endif

#ifndef SIRKIT_MILLER_RABIN_DEFINED
#define SIRKIT_MILLER_RABIN_DEFINED
template <class Integer, class Bases>
inline bool MillerRabin(Integer n, const Bases &bases) noexcept(reinsirk::detail::miller_rabin_bases_nothrow<Bases>)
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

template <class Integer>
inline bool MillerRabin(Integer n, std::initializer_list<std::uint64_t> bases) noexcept
{
    return MillerRabin<Integer, std::initializer_list<std::uint64_t>>(n, bases);
}
#endif

#ifndef SIRKIT_PRIMALITY_FILTER_DEFINED
#define SIRKIT_PRIMALITY_FILTER_DEFINED
namespace reinsirk::detail {
// 105の各剰余が3・5・7のいずれでも割り切れなければ、そのbitを1にする。
inline constexpr std::uint64_t primality_coprime_105[2] = {
    0x6c30da65a4cb2916ULL, 0x000001a2534c9699ULL};
} // namespace reinsirk::detail
#endif

#ifndef SIRKIT_IS_PRIME_DEFINED
#define SIRKIT_IS_PRIME_DEFINED
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
    if ((value & 1u) == 0) return false;
    // 32bitを超える値は小素数3・5・7をまとめて除外する。
    const auto residue = value % 105u;
    if (((reinsirk::detail::primality_coprime_105[residue >> 6] >> (residue & 63)) & 1u) == 0 ||
        value % 11u == 0 || value % 13u == 0 || value % 17u == 0)
        return false;
    if (value < std::uint64_t{4'759'123'141})
        return MillerRabin(value, {2, 7, 61});
    if (value < (std::uint64_t{1} << 62))
        return reinsirk::detail::MillerRabin64<true>(value).test_deterministic();
    return reinsirk::detail::MillerRabin64<false>(value).test_deterministic();
}
#endif

#ifndef SIRKIT_NEXT_PREV_PRIME_DEFINED
#define SIRKIT_NEXT_PREV_PRIME_DEFINED
template<class Integer>
inline std::optional<std::uint64_t> next_prime(Integer n) noexcept {
    static_assert(std::is_integral_v<Integer> && sizeof(Integer) <= sizeof(std::uint64_t),
                  "next_prime: n must be an integer of at most 64 bits");
    if constexpr (std::is_signed_v<Integer>) if (n <= 2) return std::uint64_t{2};
    auto value = static_cast<std::uint64_t>(n);
    if (value <= 2) return std::uint64_t{2};
    if ((value & 1u) == 0) ++value;
    for (;;) {
        if (is_prime(value)) return value;
        if (value > std::numeric_limits<std::uint64_t>::max() - 2) return std::nullopt;
        value += 2;
    }
}

template<class Integer>
inline std::optional<std::uint64_t> prev_prime(Integer n) noexcept {
    static_assert(std::is_integral_v<Integer> && sizeof(Integer) <= sizeof(std::uint64_t),
                  "prev_prime: n must be an integer of at most 64 bits");
    if constexpr (std::is_signed_v<Integer>) if (n < 2) return std::nullopt;
    auto value = static_cast<std::uint64_t>(n);
    if (value < 2) return std::nullopt;
    if (value == 2) return std::uint64_t{2};
    if ((value & 1u) == 0) --value;
    for (; value >= 3; value -= 2) if (is_prime(value)) return value;
    return std::uint64_t{2};
}
#endif


// Library: Math/Eratosthenes.cpp
// SPDX-License-Identifier: CC0-1.0
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/prime_sieve.md
//
// API一覧
// [PrimeSieve]
// PrimeSieve(limit=0): [0,limit]の素数表を構築する。O(limit log log limit)、奇数1個につき1bit。
// コピーは独立した表を持ち、移動元はlimit=0の空の素数表として再利用できる。
// is_prime(n): 0<=n<=limitの素数判定をO(1)で返す。
// limit(): 構築時の上限を返す。O(1)。
// prime_count(): 上限以下の素数の個数を返す。O(limit/64)。
// primes(): 上限以下の素数を昇順のvectorで返す。O(limit/64+素数数)。
// append_to(ans): 上限以下の素数を昇順でansの末尾に追加する。O(limit/64+素数数)、償却。
// for_each_prime(f): 昇順にf(uint64_tの素数)を呼ぶ。O(limit/64+素数数)、呼出し中に表を変更しない。
//
// [eratosthenes]
// eratosthenes(limit, ans): limit以下の素数を昇順でansの末尾へ追加する。O(limit log log limit)。
// eratosthenes(limit): limit以下の素数を昇順のvectorで返す。同じ計算量。

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>


#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
#include <immintrin.h>
#endif

#ifndef SIRKIT_PRIME_SIEVE_POPCOUNT_DEFINED
#define SIRKIT_PRIME_SIEVE_POPCOUNT_DEFINED
namespace reinsirk::detail {
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
// 内部用。32語単位で集計し、残りを4語・1語ずつ処理する。
[[gnu::target("avx2"), gnu::noinline]] inline std::uint64_t popcount_words_avx2(
    const std::uint64_t* words, std::size_t count) noexcept {
    const auto table = _mm256_setr_epi8(
        0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
        0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4);
    const auto mask = _mm256_set1_epi8(15);
    const auto zero = _mm256_setzero_si256();
    auto sum = zero;
    std::size_t i = 0;
    for (; count - i >= 32; i += 32) {
        auto bytes = zero;
        for (std::size_t j = 0; j < 32; j += 4) {
            const auto value = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(words + i + j));
            const auto low = _mm256_and_si256(value, mask);
            const auto high = _mm256_and_si256(_mm256_srli_epi16(value, 4), mask);
            const auto bits = _mm256_add_epi8(_mm256_shuffle_epi8(table, low),
                                             _mm256_shuffle_epi8(table, high));
            bytes = _mm256_add_epi8(bytes, bits);
        }
        sum = _mm256_add_epi64(sum, _mm256_sad_epu8(bytes, zero));
    }
    for (; count - i >= 4; i += 4) {
        const auto value = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(words + i));
        const auto low = _mm256_and_si256(value, mask);
        const auto high = _mm256_and_si256(_mm256_srli_epi16(value, 4), mask);
        const auto bits = _mm256_add_epi8(_mm256_shuffle_epi8(table, low),
                                         _mm256_shuffle_epi8(table, high));
        sum = _mm256_add_epi64(sum, _mm256_sad_epu8(bits, zero));
    }
    alignas(32) std::uint64_t lanes[4];
    _mm256_store_si256(reinterpret_cast<__m256i*>(lanes), sum);
    std::uint64_t result = lanes[0] + lanes[1] + lanes[2] + lanes[3];
    for (; i < count; ++i) result += std::popcount(words[i]);
    return result;
}
#endif
} // namespace reinsirk::detail
#endif


#ifndef SIRKIT_PRIME_SIEVE_PATTERN_DEFINED
#define SIRKIT_PRIME_SIEVE_PATTERN_DEFINED
namespace reinsirk::detail {
inline constexpr auto sieve_pattern = [] {
    std::array<std::uint64_t, 1155> values{};
    for (std::size_t word = 0; word < values.size(); ++word)
        for (unsigned bit = 0; bit < 64; ++bit) {
            const auto n = (word * 64 + bit) * 2 + 1;
            if (n % 3 != 0 && n % 5 != 0 && n % 7 != 0 && n % 11 != 0)
                values[word] |= std::uint64_t{1} << bit;
        }
    return values;
}();
inline void fill_sieve_pattern(std::uint64_t* dest, std::size_t count) noexcept {
    constexpr std::size_t period = sieve_pattern.size();
    while (count >= period) {
        std::copy(sieve_pattern.begin(), sieve_pattern.end(), dest);
        dest += period; count -= period;
    }
    std::copy_n(sieve_pattern.begin(), count, dest);
}
}
#endif
#ifndef SIRKIT_PRIME_SIEVE_DEFINED
#define SIRKIT_PRIME_SIEVE_DEFINED

class PrimeSieve {
public:
    explicit PrimeSieve(std::uint64_t limit = 0) : limit_(limit) {
        if (limit < 128) {
            if (limit != 0) {
                // 127以下の奇合成数は3・5・7・11のいずれかの倍数。
                constexpr auto primes = (reinsirk::detail::sieve_pattern[0] |
                    (std::uint64_t{1} << 1) | (std::uint64_t{1} << 2) |
                    (std::uint64_t{1} << 3) | (std::uint64_t{1} << 5)) & ~std::uint64_t{1};
                const auto bits = static_cast<unsigned>((limit + 1) / 2);
                flags_.push_back(primes & (~std::uint64_t{0} >> (64 - bits)));
            }
            return;
        }
        const std::uint64_t odds = limit / 2 + limit % 2;
        const std::uint64_t words = odds / 64 + (odds % 64 != 0);
        if (words > flags_.max_size())
            throw std::length_error("PrimeSieve: limit exceeds capacity");
        flags_.assign(static_cast<std::size_t>(words), ~std::uint64_t{0});
        build_large(odds);
    }

    PrimeSieve(const PrimeSieve&) = default;
    PrimeSieve& operator=(const PrimeSieve& other) {
        flags_ = other.flags_;
        limit_ = other.limit_;
        return *this;
    }
    PrimeSieve(PrimeSieve&& other) noexcept : limit_(std::exchange(other.limit_, 0)) {
        flags_.swap(other.flags_);
    }
    PrimeSieve& operator=(PrimeSieve&& other) noexcept {
        if (this != &other) {
            PrimeSieve next(std::move(other));
            std::swap(limit_, next.limit_);
            flags_.swap(next.flags_);
        }
        return *this;
    }

    bool is_prime(std::uint64_t n) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (n > limit_) [[unlikely]]
            throw std::out_of_range("PrimeSieve::is_prime: beyond limit");
#endif
        if ((n & 1) == 0) return n == 2;
        const std::uint64_t index = n / 2;
        return (flags_[static_cast<std::size_t>(index / 64)] >> (index % 64) & 1) != 0;
    }
    std::uint64_t limit() const noexcept { return limit_; }
    std::uint64_t prime_count() const noexcept {
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
#if !(defined(__clang__) && defined(__AVX2__))
        if (limit_ >= 3969 && __builtin_cpu_supports("avx2")) [[unlikely]]
            return static_cast<std::uint64_t>(limit_ >= 2) +
                   reinsirk::detail::popcount_words_avx2(flags_.data(), flags_.size());
#endif
#endif
        std::uint64_t count = limit_ >= 2;
        for (std::uint64_t word : flags_) count += std::popcount(word);
        return count;
    }
#if defined(__clang__) && !defined(__AVX2__)
    std::vector<std::uint64_t> primes() const {
        std::vector<std::uint64_t> result;
        if (limit_ < 2) return result;
        if (limit_ >= 16) {
            const auto count = prime_count();
            if (count > result.max_size())
                throw std::length_error("PrimeSieve::primes: capacity exceeded");
            result.reserve(static_cast<std::size_t>(count));
        }
        append_to(result);
        return result;
    }
#else
    std::vector<std::uint64_t> primes() const {
        std::vector<std::uint64_t> result;
        if (limit_ < 2) return result;
        if (limit_ >= 16384) {
            const auto count = prime_count();
            if (count > result.max_size())
                throw std::length_error("PrimeSieve::primes: capacity exceeded");
            result.reserve(static_cast<std::size_t>(count));
        } else if (limit_ >= 16) {
            return small_primes();
        }
        append_to(result);
        return result;
    }
#endif
#if defined(__clang__)
    [[gnu::always_inline]]
#endif
    void append_to(std::vector<std::uint64_t>& ans) const {
        if (limit_ >= 2) ans.push_back(2);
        for (std::size_t i = 0; i < flags_.size(); ++i) {
            std::uint64_t word = flags_[i];
            while (word != 0) {
                const unsigned bit = std::countr_zero(word);
                ans.push_back((static_cast<std::uint64_t>(i) * 64 + bit) * 2 + 1);
                word &= word - 1;
            }
        }
    }

    template <class Function>
    void for_each_prime(Function&& function) const {
        if (limit_ >= 2) function(std::uint64_t{2});
        for (std::size_t i = 0; i < flags_.size(); ++i) {
            std::uint64_t word = flags_[i];
            while (word != 0) {
                const unsigned bit = std::countr_zero(word);
                function((static_cast<std::uint64_t>(i) * 64 + bit) * 2 + 1);
                word &= word - 1;
            }
        }
    }

private:
#if !defined(__clang__) || defined(__AVX2__)
#if defined(__GNUC__) || defined(__clang__)
    [[gnu::noinline]]
#endif
    std::vector<std::uint64_t> small_primes() const {
        std::vector<std::uint64_t> result;
        const auto count = prime_count();
        if (count > result.max_size())
            throw std::length_error("PrimeSieve::primes: capacity exceeded");
        result.reserve(static_cast<std::size_t>(count));
        append_to(result);
        return result;
    }
#endif

    void build_large(std::uint64_t odds) {
        reinsirk::detail::fill_sieve_pattern(flags_.data(), flags_.size());
        flags_[0] |= (std::uint64_t{1} << 1) | (std::uint64_t{1} << 2) |
                     (std::uint64_t{1} << 3) | (std::uint64_t{1} << 5);
        flags_[0] &= ~std::uint64_t{1};
        if (odds % 64 != 0)
            flags_.back() &= (std::uint64_t{1} << (odds % 64)) - 1;
        if (limit_ >= 262144) {
            build_blocked(odds);
            return;
        }
        for (std::uint64_t p = 13; p <= limit_ / p; p += 2) {
            const auto index = p / 2;
            if (((flags_[index / 64] >> (index % 64)) & 1) == 0) continue;
            for (auto k = p * p / 2; k < odds; k += p)
                flags_[k / 64] &= ~(std::uint64_t{1} << (k % 64));
        }
    }

    // 小素数の倍数をp語周期のANDで消す。素数自身は呼出側で復元する。
    template <unsigned Prime>
#if defined(__GNUC__) || defined(__clang__)
    [[gnu::noinline]]
#endif
    static void mask_multiples(std::uint64_t* flags, std::uint64_t first_word,
                               std::uint64_t word_count) noexcept {
        static constexpr auto pattern = [] {
            std::array<std::uint64_t, Prime> result{};
            for (unsigned w = 0; w < Prime; ++w)
                for (unsigned bit = 0; bit < 64; ++bit)
                    if ((w * 128 + bit * 2 + 1) % Prime != 0)
                        result[w] |= std::uint64_t{1} << bit;
            return result;
        }();
        auto* dest = flags + first_word;
        auto phase = first_word % Prime;
        while (phase != 0 && word_count != 0) {
            *dest++ &= pattern[phase];
            --word_count;
            if (++phase == Prime) phase = 0;
        }
        while (word_count >= Prime) {
            for (unsigned j = 0; j < Prime; ++j) dest[j] &= pattern[j];
            dest += Prime;
            word_count -= Prime;
        }
        for (unsigned j = 0; j < word_count; ++j) dest[j] &= pattern[j];
    }

    void build_blocked(std::uint64_t odds) {
        std::uint64_t root = std::uint64_t{1} << ((std::bit_width(limit_) + 1) / 2);
        for (;;) {
            const auto next = (root + limit_ / root) / 2;
            if (next >= root) break;
            root = next;
        }
        const std::uint64_t base_odds = root / 2 + root % 2;
        auto* const flags = flags_.data();
        for (std::uint64_t p = 13; p <= root / p; p += 2) {
            const auto index = p / 2;
            if (((flags[index / 64] >> (index % 64)) & 1) == 0) continue;
            for (auto k = p * p / 2; k < base_odds; k += p)
                flags[k / 64] &= ~(std::uint64_t{1} << (k % 64));
        }
        struct Strike { std::uint64_t prime, next; };
        std::vector<Strike> strikes;
        for (std::uint64_t p = 37; p <= root; p += 2) {
            const auto index = p / 2;
            if ((flags[index / 64] >> (index % 64)) & 1)
                strikes.push_back({p, p * p / 2});
        }
        const std::uint64_t block_bits = std::max(std::uint64_t{262144},
            (root + 63) & ~std::uint64_t{63});
        std::size_t active = 0;
        for (std::uint64_t begin = 0; begin < odds; begin += block_bits) {
            const auto end = std::min(odds, begin + block_bits);
            const auto first_word = begin / 64;
            const auto word_count = (end + 63) / 64 - first_word;
            mask_multiples<13>(flags, first_word, word_count);
            mask_multiples<17>(flags, first_word, word_count);
            mask_multiples<19>(flags, first_word, word_count);
            mask_multiples<23>(flags, first_word, word_count);
            mask_multiples<29>(flags, first_word, word_count);
            mask_multiples<31>(flags, first_word, word_count);
            while (active < strikes.size() && strikes[active].next < end) ++active;
            for (std::size_t i = 0; i < active; ++i) {
                auto k = strikes[i].next;
                const auto step = strikes[i].prime;
                for (; k + step * 3 < end; k += step * 4) {
                    flags[k / 64] &= ~(std::uint64_t{1} << (k % 64));
                    flags[(k + step * 1) / 64] &= ~(std::uint64_t{1} << ((k + step * 1) % 64));
                    flags[(k + step * 2) / 64] &= ~(std::uint64_t{1} << ((k + step * 2) % 64));
                    flags[(k + step * 3) / 64] &= ~(std::uint64_t{1} << ((k + step * 3) % 64));
                }
                for (; k < end; k += step)
                    flags[k / 64] &= ~(std::uint64_t{1} << (k % 64));
                strikes[i].next = k;
            }
        }
        flags[0] |= (std::uint64_t{1} << 6) | (std::uint64_t{1} << 8) |
                    (std::uint64_t{1} << 9) | (std::uint64_t{1} << 11) |
                    (std::uint64_t{1} << 14) | (std::uint64_t{1} << 15);
    }

    std::uint64_t limit_;
    std::vector<std::uint64_t> flags_;
};
#endif

#ifndef SIRKIT_ERATOSTHENES_DEFINED
#define SIRKIT_ERATOSTHENES_DEFINED
inline void eratosthenes(std::uint64_t limit, std::vector<std::uint64_t>& ans) {
    PrimeSieve(limit).append_to(ans);
}
inline std::vector<std::uint64_t> eratosthenes(std::uint64_t limit) {
    return PrimeSieve(limit).primes();
}
#endif


// SPDX-License-Identifier: CC0-1.0
#include <array>
#include <iostream>
#include <stdexcept>

int main() {
    const auto require=[](bool ok) {if(!ok)throw std::runtime_error("primality reference mismatch");};
    for(std::uint64_t bound:{0,1,2,3,63,64,65,127,128,129,10000}) {
        PrimeSieve sieve(bound);std::vector<std::uint64_t> expected;
        for(std::uint64_t n=0;n<=bound;++n) {
            bool prime=n>=2;for(std::uint64_t d=2;d<=n/d;++d)if(n%d==0){prime=false;break;}
            require(sieve.is_prime(n)==prime);require(is_prime(n)==prime);if(prime)expected.push_back(n);
        }
        require(sieve.prime_count()==expected.size() && sieve.primes()==expected && eratosthenes(bound)==expected);
        std::vector<std::uint64_t> appended{123};eratosthenes(bound,appended);expected.insert(expected.begin(),123);require(appended==expected);
        const PrimeSieve copied=sieve;PrimeSieve moved=std::move(sieve);require(copied.primes()==moved.primes());require(sieve.limit()==0);
    }
    for(auto n:std::array<std::uint64_t,6>{2,998244353,4294967291ULL,2305843009213693951ULL,18446744073709551557ULL,18446744073709551533ULL}) require(is_prime(n));
    for(auto n:std::array<std::uint64_t,8>{0,1,341,2047,3215031751ULL,341550071728321ULL,3825123056546413051ULL,18446744073709551615ULL}) require(!is_prime(n));
    require(!is_prime(-1));require(MillerRabin(2047,{2}));require(!MillerRabin(2047,{2,3}));
    std::cout<<"OK\n";
}
