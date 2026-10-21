// Verify: ローカル独立検査

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


// Library: Math/Eratosthenes.cpp
// SPDX-License-Identifier: CC0-1.0

#include <bit>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#ifndef REINSIRK_PRIME_SIEVE_DEFINED
#define REINSIRK_PRIME_SIEVE_DEFINED

// PrimeSieve(limit=0): [0,limit]の素数表を構築する。O(limit log log limit)、奇数1個につき1bit。
// コピーは独立した表を持ち、移動元はlimit=0の空の素数表として再利用できる。
// is_prime(n): 0<=n<=limitの素数判定をO(1)で返す。
// limit(): 構築時の上限を返す。O(1)。
// prime_count(): 上限以下の素数の個数を返す。O(limit/64)。
// primes(): 上限以下の素数を昇順のvectorで返す。O(limit/64+素数数)。
// append_to(ans): 上限以下の素数を昇順でansの末尾に追加する。O(limit/64+素数数)、償却。
class PrimeSieve {
public:
    explicit PrimeSieve(std::uint64_t limit = 0) : limit_(limit) {
        const std::uint64_t odds = limit / 2 + limit % 2;
        const std::uint64_t words = odds / 64 + (odds % 64 != 0);
        if (words > flags_.max_size())
            throw std::length_error("PrimeSieve: limit exceeds capacity");
        flags_.assign(static_cast<std::size_t>(words), ~std::uint64_t{0});
        if (odds == 0) return;
        flags_[0] &= ~std::uint64_t{1};
        if (odds % 64 != 0)
            flags_.back() &= (std::uint64_t{1} << (odds % 64)) - 1;
        for (std::uint64_t p = 3; p <= limit / p; p += 2) {
            const std::uint64_t index = p / 2;
            if ((flags_[static_cast<std::size_t>(index / 64)] >> (index % 64) & 1) == 0)
                continue;
            // odds<=2^63、p<=sqrt(limit)なのでk+pもuint64_tに収まる。
            for (std::uint64_t k = p * p / 2; k < odds; k += p) {
                flags_[static_cast<std::size_t>(k / 64)] &= ~(std::uint64_t{1} << (k % 64));
            }
        }
    }

    PrimeSieve(const PrimeSieve&) = default;
    PrimeSieve& operator=(const PrimeSieve&) = default;
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
        std::uint64_t count = limit_ >= 2;
        for (std::uint64_t word : flags_) count += std::popcount(word);
        return count;
    }
    std::vector<std::uint64_t> primes() const {
        std::vector<std::uint64_t> result;
        append_to(result);
        return result;
    }
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

private:
    std::uint64_t limit_;
    std::vector<std::uint64_t> flags_;
};
#endif

#ifndef REINSIRK_ERATOSTHENES_DEFINED
#define REINSIRK_ERATOSTHENES_DEFINED
// eratosthenes(limit, ans): limit以下の素数を昇順でansの末尾へ追加する。O(limit log log limit)。
// eratosthenes(limit): limit以下の素数を昇順のvectorで返す。同じ計算量。
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
