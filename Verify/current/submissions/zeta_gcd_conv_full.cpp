#define SKIP_RUNTIME_CHECKS
// Verify: https://judge.yosupo.jp/problem/gcd_convolution

// Library: Math/Zeta_Mebius.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/set-function/zeta-mobius-transform.hpp.html
// 参考: https://nyaannyaan.github.io/library/multiplicative-function/divisor-multiple-transform.hpp.html
#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <stdexcept>
#include <span>
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


#ifndef SIRKIT_INTEGER_SIEVE_DEFINED
#define SIRKIT_INTEGER_SIEVE_DEFINED
// IntegerSieve(maxn): maxn>=0の最小素因数表・昇順素数列をO(maxn)時間・領域で作る。0,1の最小素因数は0。
// least_prime_factor[x]は最小素因数、prime_listは昇順素数列。表は書き換えず利用する。
// コピーは独立、コピー代入は強保証。ムーブ元は空となりmax_value()=-1。
// swap(other): 表と素数列をO(1)で交換する。
// max_value(): 表の上限を返す。is_prime(x): 0<=x<=max_value()の素数判定をO(1)で返す。
// prime_factorization(x): 1<=x<=max_value()^2の素因数と指数のmapを返す。x=1は空の表でも許す。
// enumerate_divisors(x), sorted_divisors(x): prime_factorizationと同じ前提で全約数を返す。後者だけ昇順。
// divisor_totients(x): prime_factorizationと同じ前提で各約数dからphi(d)へのmapを返す。
// mobius_table(): O(maxn)でmu表を返す。mu[0]=0,mu[1]=1。
// totient_table(): O(maxn)でphi表を返す。phi[0]=0,phi[1]=1。
// power_table<Mint>(k,nmax): 0<=nmax<=max_value(),k>=0でn^kの表。0^0=1。
// べき乗表はO(nmax+pi(nmax)log(k+1))時間・O(nmax)領域。公開配列を変更せず使う。
struct IntegerSieve {
    std::vector<int> least_prime_factor;
    std::vector<int> prime_list;

    explicit IntegerSieve(int maxn) : least_prime_factor(table_size(maxn)) {
        const std::size_t limit = least_prime_factor.size();
        for (std::size_t i = 2; i < limit; ++i) {
            if (least_prime_factor[i] == 0) {
                least_prime_factor[i] = static_cast<int>(i);
                prime_list.push_back(static_cast<int>(i));
            }
            const int first = least_prime_factor[i];
            for (const int p : prime_list) {
                const std::uint64_t next = static_cast<std::uint64_t>(i) *
                                           static_cast<std::uint64_t>(p);
                if (next >= limit) break;
                least_prime_factor[static_cast<std::size_t>(next)] = p;
                if (p == first) break;
            }
        }
    }

    IntegerSieve(const IntegerSieve&) = default;
    IntegerSieve& operator=(const IntegerSieve& other) {
        if (this != &other) {
            IntegerSieve copy(other);
            swap(copy);
        }
        return *this;
    }
    IntegerSieve(IntegerSieve&& other) noexcept
        : least_prime_factor(std::move(other.least_prime_factor)), prime_list(std::move(other.prime_list)) {
        other.least_prime_factor.clear();
        other.prime_list.clear();
    }
    IntegerSieve& operator=(IntegerSieve&& other) noexcept {
        if (this != &other) {
            least_prime_factor = std::move(other.least_prime_factor);
            prime_list = std::move(other.prime_list);
            other.least_prime_factor.clear();
            other.prime_list.clear();
        }
        return *this;
    }

    void swap(IntegerSieve& other) noexcept {
        least_prime_factor.swap(other.least_prime_factor);
        prime_list.swap(other.prime_list);
    }

    int max_value() const noexcept {
        return least_prime_factor.empty() ? -1 : static_cast<int>(least_prime_factor.size() - 1);
    }

    template <std::integral Int>
    bool is_prime(Int x) const {
#ifndef SKIP_RUNTIME_CHECKS
        if constexpr (std::signed_integral<Int>) {
            if (x < 0) throw std::out_of_range("IntegerSieve::is_prime: negative input");
        }
        if (static_cast<std::uint64_t>(x) >= least_prime_factor.size()) {
            throw std::out_of_range("IntegerSieve::is_prime: input outside table");
        }
#endif
        const std::uint64_t value = static_cast<std::uint64_t>(x);
        return value >= 2 && static_cast<std::uint64_t>(least_prime_factor[static_cast<std::size_t>(value)]) == value;
    }

    template <std::integral Int>
    std::map<Int, int> prime_factorization(Int x) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (x < 1) throw std::invalid_argument("IntegerSieve::prime_factorization: input must be positive");
#endif
        std::uint64_t rest = static_cast<std::uint64_t>(x);
        std::map<Int, int> result;
        if (rest == 1) return result;
        const int maximum = max_value();
#ifndef SKIP_RUNTIME_CHECKS
        if (maximum < 1 || rest > static_cast<std::uint64_t>(maximum) *
                                     static_cast<std::uint64_t>(maximum)) {
            throw std::out_of_range("IntegerSieve::prime_factorization: input exceeds table limit squared");
        }
#endif
        const std::uint64_t bound = static_cast<std::uint64_t>(maximum);
        if (rest > bound) {
            for (const int prime : prime_list) {
                const std::uint64_t p = static_cast<std::uint64_t>(prime);
                if (p * p > rest) break;
                if (rest % p == 0) {
                    int exponent = 0;
                    do {
                        rest /= p;
                        ++exponent;
                    } while (rest % p == 0);
                    result.emplace(static_cast<Int>(prime), exponent);
                    if (rest <= bound) break;
                }
            }
            if (rest > bound) {
                result.emplace(static_cast<Int>(rest), 1);
                return result;
            }
        }
        while (rest > 1) {
            const int prime = least_prime_factor[static_cast<std::size_t>(rest)];
            int exponent = 0;
            do {
                rest /= static_cast<std::uint64_t>(prime);
                ++exponent;
            } while (rest > 1 && least_prime_factor[static_cast<std::size_t>(rest)] == prime);
            result.emplace(static_cast<Int>(prime), exponent);
        }
        return result;
    }

    template <std::integral Int>
    std::vector<Int> enumerate_divisors(Int x) const {
        const auto factors = prime_factorization(x);
        std::vector<Int> result{static_cast<Int>(1)};
        for (const auto& [prime, exponent] : factors) {
            const std::size_t previous = result.size();
            reserve_multiple(result, static_cast<std::size_t>(exponent) + 1);
            std::uint64_t power = 1;
            for (int e = 1; e <= exponent; ++e) {
                power *= static_cast<std::uint64_t>(prime);
                for (std::size_t i = 0; i < previous; ++i) {
                    result.push_back(static_cast<Int>(static_cast<std::uint64_t>(result[i]) * power));
                }
            }
        }
        return result;
    }

    template <std::integral Int>
    std::vector<Int> sorted_divisors(Int x) const {
        auto result = enumerate_divisors(x);
        std::sort(result.begin(), result.end());
        return result;
    }

    template <std::integral Int>
    std::map<Int, Int> divisor_totients(Int x) const {
        const auto factors = prime_factorization(x);
        std::vector<std::pair<Int, Int>> values{{static_cast<Int>(1), static_cast<Int>(1)}};
        for (const auto& [prime, exponent] : factors) {
            const std::size_t previous = values.size();
            reserve_multiple(values, static_cast<std::size_t>(exponent) + 1);
            const std::uint64_t p = static_cast<std::uint64_t>(prime);
            std::uint64_t power = 1;
            std::uint64_t phi_power = p - 1;
            for (int e = 1; e <= exponent; ++e) {
                power *= p;
                for (std::size_t i = 0; i < previous; ++i) {
                    values.emplace_back(
                        static_cast<Int>(static_cast<std::uint64_t>(values[i].first) * power),
                        static_cast<Int>(static_cast<std::uint64_t>(values[i].second) * phi_power));
                }
                if (e < exponent) phi_power *= p;
            }
        }
        return std::map<Int, Int>(values.begin(), values.end());
    }

    std::vector<int> mobius_table() const {
        std::vector<int> result(least_prime_factor.size());
        if (result.size() > 1) result[1] = 1;
        for (std::size_t i = 2; i < result.size(); ++i) {
            const int p = least_prime_factor[i];
            const std::size_t quotient = i / static_cast<std::size_t>(p);
            result[i] = least_prime_factor[quotient] == p ? 0 : -result[quotient];
        }
        return result;
    }

    std::vector<int> totient_table() const {
        std::vector<int> result(least_prime_factor.size());
        if (result.size() > 1) result[1] = 1;
        for (std::size_t i = 2; i < result.size(); ++i) {
            const int p = least_prime_factor[i];
            const std::size_t quotient = i / static_cast<std::size_t>(p);
            result[i] = result[quotient] * (least_prime_factor[quotient] == p ? p : p - 1);
        }
        return result;
    }

    template <class MODINT>
    std::vector<MODINT> power_table(std::int64_t k, int nmax) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (k < 0) throw std::invalid_argument("IntegerSieve::power_table: negative exponent");
        if (nmax < 0 || nmax > max_value()) {
            throw std::out_of_range("IntegerSieve::power_table: input outside table");
        }
#endif
        const std::size_t count = table_size(nmax);
        if (count > std::vector<MODINT>().max_size()) {
            throw std::length_error("IntegerSieve::power_table: table too large");
        }
        if (k == 0) return std::vector<MODINT>(count, MODINT(1));
        std::vector<MODINT> result(count);
        if (count > 0) result[0] = MODINT(0);
        if (count > 1) result[1] = MODINT(1);
        for (std::size_t i = 2; i < count; ++i) {
            const int p = least_prime_factor[i];
            if (static_cast<std::size_t>(p) == i) {
                result[i] = MODINT(p).pow(k);
            } else {
                result[i] = result[static_cast<std::size_t>(p)] *
                            result[i / static_cast<std::size_t>(p)];
            }
        }
        return result;
    }

private:
    static std::size_t table_size(int maximum) {
#ifndef SKIP_RUNTIME_CHECKS
        if (maximum < 0) throw std::invalid_argument("IntegerSieve: negative table limit");
#endif
        const std::uint64_t count = static_cast<std::uint64_t>(maximum) + 1;
        if (count > std::vector<int>().max_size()) {
            throw std::length_error("IntegerSieve: table too large");
        }
        return static_cast<std::size_t>(count);
    }

    template <class T>
    static void reserve_multiple(std::vector<T>& values, std::size_t multiplier) {
        if (multiplier > values.max_size() / values.size()) {
            throw std::length_error("IntegerSieve: divisor table too large");
        }
        values.reserve(values.size() * multiplier);
    }
};
#endif

#ifndef SIRKIT_INTEGER_TRANSFORM_DETAIL_DEFINED
#define SIRKIT_INTEGER_TRANSFORM_DETAIL_DEFINED
namespace reinsirk::detail {
struct IntegerTransform {
    // 素数pについて、指数を1段進める／戻す伝播を行う。
    template <bool ToMultiples, bool Subtract, class T, class PrimeRange>
    static void propagate(std::vector<T>& a, const PrimeRange& primes) {
        if (a.size() <= 2) return;
        const std::size_t upper = a.size() - 1;
        for (const int value : primes) {
            const auto p = static_cast<std::size_t>(value);
            if (p > upper) break;
            const auto count = upper / p;
            if constexpr (ToMultiples != Subtract) {
                for (std::size_t from = 1, to = p; from <= count; ++from, to += p) {
                    if constexpr (ToMultiples) a[to] += a[from];
                    else a[from] -= a[to];
                }
            } else {
                for (std::size_t from = count, to = count * p; from; --from, to -= p) {
                    if constexpr (ToMultiples) a[to] -= a[from];
                    else a[from] += a[to];
                }
            }
        }
    }
    // 小さい範囲は定数表を使い、素数表の確保と初期化を省く。
    static std::span<const int> prime_view_for(std::size_t size, std::vector<int>& storage) {
        static constexpr std::array<int, 31> small_primes{
            2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53,
            59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127
        };
        if (size <= 128) {
            const auto end = std::lower_bound(small_primes.begin(), small_primes.end(), static_cast<int>(size));
            return {small_primes.data(), static_cast<std::size_t>(end - small_primes.begin())};
        }
        storage = primes_for(size);
        return storage;
    }
    static std::vector<int> primes_for(std::size_t size) {
        if (size <= 2) return {};
        if (size - 1 > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("integer transform: maximum exceeds INT_MAX");
        const auto upper = size - 1;
        std::vector<unsigned char> marked(upper / 2 + 1);
        std::vector<int> answer{2};
        for (std::size_t p = 3; p <= upper; p += 2) {
            if (marked[p / 2]) continue;
            answer.push_back(static_cast<int>(p));
            if (p <= upper / p)
                for (std::size_t k = p * p / 2; k < marked.size(); k += p) marked[k] = 1;
        }
        return answer;
    }
#ifndef SKIP_RUNTIME_CHECKS
    static void require_sieve(std::size_t size, const IntegerSieve& sieve) {
        if (size && (sieve.max_value() < 0 || size - 1 > static_cast<std::size_t>(sieve.max_value())))
            throw std::out_of_range("integer transform: sieve too small");
    }
    static void require_same_size(std::size_t a, std::size_t b) {
        if (a != b) throw std::invalid_argument("integer convolution: different lengths");
    }
#endif
};
} // namespace reinsirk::detail
#endif
// 正の添字の4変換。f[0]は不変、空配列も可。時間O(n log log n)。
// multiple_zeta_transform(f): f[d]=sum_{d|k} f[k]へ更新する。
// multiple_mobius_transform(f): 倍数ゼータ変換を元に戻す。
// divisor_zeta_transform(f): f[k]=sum_{d|k} f[d]へ更新する。
// divisor_mobius_transform(f): 約数ゼータ変換を元に戻す。
// 各(f,sieve)版: sieve.max_value()>=f.size()-1の素数表を再利用し、追加領域O(1)。
// 各(f)版: 素数表も作る。f.size()-1<=INT_MAX、追加領域O(n)。
#ifndef SIRKIT_MULTIPLE_ZETA_DEFINED
#define SIRKIT_MULTIPLE_ZETA_DEFINED
template <class T> void multiple_zeta_transform(std::vector<T>& f, const IntegerSieve& sieve) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_sieve(f.size(), sieve);
#endif
    reinsirk::detail::IntegerTransform::propagate<false, false>(f, sieve.prime_list);
}
template <class T> void multiple_zeta_transform(std::vector<T>& f) {
    std::vector<int> storage;
    const auto primes = reinsirk::detail::IntegerTransform::prime_view_for(f.size(), storage);
    reinsirk::detail::IntegerTransform::propagate<false, false>(f, primes);
}
#endif
#ifndef SIRKIT_MULTIPLE_MOEBIUS_DEFINED
#define SIRKIT_MULTIPLE_MOEBIUS_DEFINED
template <class T> void multiple_mobius_transform(std::vector<T>& f, const IntegerSieve& sieve) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_sieve(f.size(), sieve);
#endif
    reinsirk::detail::IntegerTransform::propagate<false, true>(f, sieve.prime_list);
}
template <class T> void multiple_mobius_transform(std::vector<T>& f) {
    std::vector<int> storage;
    const auto primes = reinsirk::detail::IntegerTransform::prime_view_for(f.size(), storage);
    reinsirk::detail::IntegerTransform::propagate<false, true>(f, primes);
}
#endif
#ifndef SIRKIT_DIVISOR_ZETA_DEFINED
#define SIRKIT_DIVISOR_ZETA_DEFINED
template <class T> void divisor_zeta_transform(std::vector<T>& f, const IntegerSieve& sieve) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_sieve(f.size(), sieve);
#endif
    reinsirk::detail::IntegerTransform::propagate<true, false>(f, sieve.prime_list);
}
template <class T> void divisor_zeta_transform(std::vector<T>& f) {
    std::vector<int> storage;
    const auto primes = reinsirk::detail::IntegerTransform::prime_view_for(f.size(), storage);
    reinsirk::detail::IntegerTransform::propagate<true, false>(f, primes);
}
#endif
#ifndef SIRKIT_DIVISOR_MOEBIUS_DEFINED
#define SIRKIT_DIVISOR_MOEBIUS_DEFINED
template <class T> void divisor_mobius_transform(std::vector<T>& f, const IntegerSieve& sieve) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_sieve(f.size(), sieve);
#endif
    reinsirk::detail::IntegerTransform::propagate<true, true>(f, sieve.prime_list);
}
template <class T> void divisor_mobius_transform(std::vector<T>& f) {
    std::vector<int> storage;
    const auto primes = reinsirk::detail::IntegerTransform::prime_view_for(f.size(), storage);
    reinsirk::detail::IntegerTransform::propagate<true, true>(f, primes);
}
#endif

#ifndef SIRKIT_INTEGER_CONVOLUTION_DETAIL_DEFINED
#define SIRKIT_INTEGER_CONVOLUTION_DETAIL_DEFINED
namespace reinsirk::detail {
struct IntegerConvolution {
    template <bool Gcd, class T, class PrimeRange>
    static std::vector<T> compute(std::vector<T> a, std::vector<T> b, const PrimeRange& primes) {
        if (a.empty()) return a;
        const T left_zero = a.front(), right_zero = b.front();
        bool zero_terms_needed = true;
        if constexpr (std::equality_comparable<T>)
            zero_terms_needed = left_zero != T{} || right_zero != T{};
        if constexpr (Gcd) {
            std::vector<T> mixed;
            if (zero_terms_needed) {
                mixed.resize(a.size());
                for (std::size_t k = 1; k < a.size(); ++k)
                    mixed[k] = left_zero * b[k] + a[k] * right_zero;
            }
            reinsirk::detail::IntegerTransform::propagate<false, false>(a, primes);
            reinsirk::detail::IntegerTransform::propagate<false, false>(b, primes);
            for (std::size_t k = 0; k < a.size(); ++k) a[k] *= b[k];
            reinsirk::detail::IntegerTransform::propagate<false, true>(a, primes);
            if (zero_terms_needed)
                for (std::size_t k = 1; k < a.size(); ++k) a[k] += mixed[k];
        } else {
            T at_zero = left_zero * right_zero;
            if (zero_terms_needed) {
                T left_sum{}, right_sum{};
                for (std::size_t k = 1; k < a.size(); ++k) {
                    left_sum += a[k]; right_sum += b[k];
                }
                at_zero += left_zero * right_sum + left_sum * right_zero;
            }
            reinsirk::detail::IntegerTransform::propagate<true, false>(a, primes);
            reinsirk::detail::IntegerTransform::propagate<true, false>(b, primes);
            for (std::size_t k = 1; k < a.size(); ++k) a[k] *= b[k];
            reinsirk::detail::IntegerTransform::propagate<true, true>(a, primes);
            a.front() = at_zero;
        }
        return a;
    }
};
} // namespace reinsirk::detail
#endif
// gcd_convolution(f,g): h[k]=sum_{gcd(i,j)=k}f[i]g[j]。gcd(0,k)=k（gcd(0,0)=0）。
// lcm_convolution(f,g): h[k]=sum_{lcm(i,j)=k}f[i]g[j]。lcm(0,k)=0、結果k>=nは切り捨て。
// 両者は同じ長さnの入力（空も可）、O(n log log n)時間・O(n)追加領域。
// 各(f,g,sieve)版: sieve.max_value()>=n-1の素数表を再利用する。
// 各(f,g)版: 素数表も作る。n-1<=INT_MAX。入力値・全中間値がTの範囲に収まること。
#ifndef SIRKIT_GCDCONV_DEFINED
#define SIRKIT_GCDCONV_DEFINED
template <class T> std::vector<T> gcd_convolution(std::vector<T> f, std::vector<T> g, const IntegerSieve& sieve) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_same_size(f.size(), g.size());
    reinsirk::detail::IntegerTransform::require_sieve(f.size(), sieve);
#endif
    return reinsirk::detail::IntegerConvolution::compute<true>(std::move(f), std::move(g), sieve.prime_list);
}
template <class T> std::vector<T> gcd_convolution(std::vector<T> f, std::vector<T> g) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_same_size(f.size(), g.size());
#endif
    std::vector<int> storage;
    const auto primes = reinsirk::detail::IntegerTransform::prime_view_for(f.size(), storage);
    return reinsirk::detail::IntegerConvolution::compute<true>(std::move(f), std::move(g), primes);
}
#endif
#ifndef SIRKIT_LCMCONV_DEFINED
#define SIRKIT_LCMCONV_DEFINED
template <class T> std::vector<T> lcm_convolution(std::vector<T> f, std::vector<T> g, const IntegerSieve& sieve) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_same_size(f.size(), g.size());
    reinsirk::detail::IntegerTransform::require_sieve(f.size(), sieve);
#endif
    return reinsirk::detail::IntegerConvolution::compute<false>(std::move(f), std::move(g), sieve.prime_list);
}
template <class T> std::vector<T> lcm_convolution(std::vector<T> f, std::vector<T> g) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_same_size(f.size(), g.size());
#endif
    std::vector<int> storage;
    const auto primes = reinsirk::detail::IntegerTransform::prime_view_for(f.size(), storage);
    return reinsirk::detail::IntegerConvolution::compute<false>(std::move(f), std::move(g), primes);
}
#endif

#ifndef SIRKIT_INTERNAL_SPARSE_INTEGER_TRANSFORM_DEFINED
#define SIRKIT_INTERNAL_SPARSE_INTEGER_TRANSFORM_DEFINED
namespace reinsirk::detail {
// 内部用。密な範囲は位置配列、疎な範囲は素数別の伝播列を使う。
template <bool Inverse, std::integral Int, class Val>
void sparse_integer_transform(std::vector<std::pair<Int, Val>>& f,
                                      const IntegerSieve& sieve) {
    if (f.empty()) return;
    std::sort(f.begin(), f.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
#ifndef SKIP_RUNTIME_CHECKS
    if (sieve.max_value() < 0 || f.front().first <= 0
        || static_cast<std::uintmax_t>(f.back().first)
            > static_cast<std::uintmax_t>(sieve.max_value())) {
        throw std::out_of_range("sparse integer transform: key out of range");
    }
#endif
    const std::size_t m = f.size();
    const auto largest = static_cast<std::uintmax_t>(f.back().first);
    constexpr auto absent = std::numeric_limits<std::size_t>::max();
    auto propagate = [&](std::size_t src, std::size_t dst) {
        if constexpr (Inverse) f[dst].second -= f[src].second;
        else f[dst].second += f[src].second;
    };

    // 範囲をmの4倍以下に制限し、値域に比例する記憶量を避ける。
    if (largest <= std::numeric_limits<std::size_t>::max() - 1
        && (largest / m < 4 || (largest / m == 4 && largest % m == 0))) {
        const auto n = static_cast<std::size_t>(largest);
        std::vector<std::size_t> position;
        if (n >= position.max_size()) {
            throw std::length_error("sparse integer transform: position capacity");
        }
        position.assign(n + 1, absent);
        for (std::size_t i = 0; i < m; ++i) {
            const auto key = static_cast<std::size_t>(f[i].first);
#ifndef SKIP_RUNTIME_CHECKS
            if (position[key] != absent) {
                throw std::invalid_argument("sparse integer transform: duplicate key");
            }
#endif
            position[key] = i;
        }
        for (const auto raw_prime : sieve.prime_list) {
            const auto p = static_cast<std::size_t>(raw_prime);
            if (p > n) break;
            const auto limit = n / p;
            auto visit = [&](std::size_t q) {
                const auto src = position[q * p];
                if (src == absent) return;
                const auto dst = position[q];
#ifndef SKIP_RUNTIME_CHECKS
                if (dst == absent) {
                    throw std::invalid_argument("sparse integer transform: missing divisor");
                }
#endif
                propagate(src, dst);
            };
            if constexpr (Inverse) {
                for (std::size_t q = 1; q <= limit; ++q) visit(q);
            } else {
                for (std::size_t q = limit; q != 0; --q) visit(q);
            }
        }
        return;
    }

    struct Transfer {
        std::size_t src, dst, next;
    };
    std::vector<std::size_t> head(m, absent);
    std::vector<Transfer> transfers;
    if (m <= transfers.max_size() / 4) transfers.reserve(m * 4);
    else transfers.reserve(m);
    // 最大8回だけ探索し、衝突が続けば二分探索へ戻す。
    std::vector<std::size_t> slots;
    if (m > slots.max_size() / 4) {
        throw std::length_error("sparse integer transform: lookup capacity");
    }
    const auto slot_count = std::bit_ceil(m * 2);
    slots.assign(slot_count, absent);
    const auto mask = slot_count - 1;
    const auto hash_shift = std::numeric_limits<std::uint64_t>::digits
        - std::bit_width(mask);
    auto hash_key = [&](std::uintmax_t key) {
        const auto h = static_cast<std::uint64_t>(key) * UINT64_C(0x9e3779b97f4a7c15);
        return static_cast<std::size_t>(h >> hash_shift);
    };
    std::array<std::size_t, 64> small_position;
    small_position.fill(absent);
    for (std::size_t i = 0; i < m; ++i) {
#ifndef SKIP_RUNTIME_CHECKS
        if (i != 0 && f[i - 1].first == f[i].first) {
            throw std::invalid_argument("sparse integer transform: duplicate key");
        }
#endif
        const auto key = static_cast<std::uintmax_t>(f[i].first);
        if (key < small_position.size()) small_position[static_cast<std::size_t>(key)] = i;
        auto at = hash_key(key);
        for (unsigned probe = 0; probe != 8; ++probe) {
            if (slots[at] == absent) {
                slots[at] = i;
                break;
            }
            at = (at + 1) & mask;
        }
    }
    auto locate = [&](std::uintmax_t key) {
        if (key < small_position.size()) {
            const auto index = small_position[static_cast<std::size_t>(key)];
#ifndef SKIP_RUNTIME_CHECKS
            if (index == absent) {
                throw std::invalid_argument("sparse integer transform: missing divisor");
            }
#endif
            return index;
        }
        auto at = hash_key(key);
        for (unsigned probe = 0; probe != 8; ++probe) {
            const auto index = slots[at];
            if (index == absent) break;
            if (static_cast<std::uintmax_t>(f[index].first) == key) return index;
            at = (at + 1) & mask;
        }
        const auto it = std::lower_bound(f.begin(), f.end(), key,
            [](const auto& item, std::uintmax_t value) {
                return static_cast<std::uintmax_t>(item.first) < value;
            });
#ifndef SKIP_RUNTIME_CHECKS
        if (it == f.end() || static_cast<std::uintmax_t>(it->first) != key) {
            throw std::invalid_argument("sparse integer transform: missing divisor");
        }
#endif
        return static_cast<std::size_t>(it - f.begin());
    };
    for (std::size_t step = 0; step < m; ++step) {
        // 先頭への連結でゼータは降順、メビウスは昇順に伝播する。
        const auto i = Inverse ? m - 1 - step : step;
        using Factor = std::make_unsigned_t<decltype(sieve.max_value())>;
        const auto original = static_cast<Factor>(f[i].first);
        auto remaining = original;
        while (remaining > 1) {
            const auto p = static_cast<Factor>(
                sieve.least_prime_factor[static_cast<std::size_t>(remaining)]);
            const auto prime_index = locate(p);
            const auto dst = locate(original / p);
            if (transfers.size() == transfers.max_size()) {
                throw std::length_error("sparse integer transform: transfer capacity");
            }
            transfers.push_back({i, dst, head[prime_index]});
            head[prime_index] = transfers.size() - 1;
            do remaining /= p;
            while (remaining > 1
                && static_cast<Factor>(sieve.least_prime_factor[static_cast<std::size_t>(remaining)]) == p);
        }
    }
    for (const auto first : head) {
        for (auto at = first; at != absent; at = transfers[at].next) {
            const auto& transfer = transfers[at];
            propagate(transfer.src, transfer.dst);
        }
    }
}
} // namespace reinsirk::detail
#endif

#ifndef SIRKIT_SPARSE_FAST_INTEGER_ZETA_DEFINED
#define SIRKIT_SPARSE_FAST_INTEGER_ZETA_DEFINED
// m=f.size()、rは各keyの素因数を重複込みで数えた総数（以下の両API共通）。
// 正で一意なkeyの全約数を含むfをkey昇順にし、各値を倍数和へ置換。key<=sieve.max_value()、Valは+=対応。O((m+r)log(m+1))時間、O(m+r)補助領域。
template <std::integral Int, class Val>
void sparse_multiple_zeta_transform(std::vector<std::pair<Int, Val>>& f,
                              const IntegerSieve& sieve) {
    reinsirk::detail::sparse_integer_transform<false>(f, sieve);
}
#endif

#ifndef SIRKIT_SPARSE_FAST_INTEGER_MOEBIUS_DEFINED
#define SIRKIT_SPARSE_FAST_INTEGER_MOEBIUS_DEFINED
// 正で一意なkeyの全約数を含むfをkey昇順にし、倍数和の逆変換を実行。key<=sieve.max_value()、Valは-=対応。O((m+r)log(m+1))時間、O(m+r)補助領域。
template <std::integral Int, class Val>
void sparse_multiple_mobius_transform(std::vector<std::pair<Int, Val>>& f,
                                 const IntegerSieve& sieve) {
    reinsirk::detail::sparse_integer_transform<true>(f, sieve);
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
    int n{}; verify_in >> n;
    std::vector<VerifyMod998> a(n + 1), b(n + 1);
    for (int i = 1; i <= n; ++i) { std::int64_t x{}; verify_in >> x; a[i] = x; }
    for (int i = 1; i <= n; ++i) { std::int64_t x{}; verify_in >> x; b[i] = x; }
    IntegerSieve sieve(n);
    const auto c = gcd_convolution(a, b, sieve);
    for (int i = 1; i <= n; ++i) verify_out << c[i].value << (i == n ? '\n' : ' ');
}
