// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/multiplicative-function/divisor-multiple-transform.hpp.html
#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <span>
#include <utility>
#include <vector>

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

