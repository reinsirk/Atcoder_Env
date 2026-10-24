#define SKIP_RUNTIME_CHECKS
// Verify: https://judge.yosupo.jp/problem/gcd_convolution
// Submission: https://judge.yosupo.jp/submission/412861

// Library: Math/Zeta/Gcd_Lcm_Convolution.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/multiplicative-function/divisor-multiple-transform.hpp.html
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/zeta_mobius.md
//
// API一覧
// [IntegerSieve]
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
//
// [GCD／LCM畳み込み]
// gcd_convolution(f,g): h[k]=sum_{gcd(i,j)=k}f[i]g[j]。gcd(0,k)=k（gcd(0,0)=0）。
// lcm_convolution(f,g): h[k]=sum_{lcm(i,j)=k}f[i]g[j]。lcm(0,k)=0、結果k>=nは切り捨て。
// 両者は同じ長さnの入力（空も可）、O(1+n log log(n+2))時間・O(n+1)領域。値渡しでlvalue入力は保持。
// 各(f,g,sieve)版: sieve.max_value()>=n-1の素数表を再利用する。
// 各(f,g)版: 素数表も作る。n-1<=INT_MAX。T{}は零、環の加減乗算に対応（bool以外）。
// 符号付き整数の全途中結果はTの範囲内、符号なし整数は型の幅の剰余演算。
//
// gcd_square(f,sieve)/gcd_square(f): gcd_convolution(f,f)と同じ結果を返す。空も可、値渡し入力1本・変換2回。
// 時間O(1+n log log(n+2))・領域O(n+1)。表あり版はmax_value()>=n-1、表なし版はn-1<=INT_MAX。
//
// lcm_square(f,sieve)/lcm_square(f): lcm_convolution(f,f)と同じ結果を返す。空も可、値渡し入力1本・変換2回。
// 時間O(1+n log log(n+2))・領域O(n+1)。表あり版はmax_value()>=n-1、表なし版はn-1<=INT_MAX。

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef SIRKIT_INTEGER_SIEVE_DEFINED
#define SIRKIT_INTEGER_SIEVE_DEFINED
struct IntegerSieve {
    std::vector<int> least_prime_factor;
    std::vector<int> prime_list;

    explicit IntegerSieve(int maxn) : least_prime_factor(table_size(maxn), 2) {
        const std::size_t limit = least_prime_factor.size();
        least_prime_factor[0] = 0;
        if (limit > 1) least_prime_factor[1] = 0;
        if (limit > 2) prime_list.push_back(2);
        std::size_t i = 3;
        for (; i <= (limit - 1) / 3; i += 2) {
            if (least_prime_factor[i] == 2) {
                least_prime_factor[i] = static_cast<int>(i);
                prime_list.push_back(static_cast<int>(i));
            }
            const int first = least_prime_factor[i];
            for (std::size_t j = 1; j < prime_list.size(); ++j) {
                const int prime = prime_list[j];
                const std::uint64_t next = static_cast<std::uint64_t>(i) * static_cast<std::uint64_t>(prime);
                if (next >= limit) break;
                least_prime_factor[static_cast<std::size_t>(next)] = prime;
                if (prime == first) break;
            }
        }
        for (; i < limit; i += 2) {
            if (least_prime_factor[i] == 2) {
                least_prime_factor[i] = static_cast<int>(i);
                prime_list.push_back(static_cast<int>(i));
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
#if defined(__GNUC__) && !defined(__clang__)
    template <bool ToMultiples, bool Subtract, class T, class PrimeRange>
    static void propagate(std::vector<T>& a, const PrimeRange& primes) {
        if constexpr (std::is_trivially_copyable_v<T> && sizeof(T) <= 8) {
            if (a.size() >= 257) {
                propagate_expanded<ToMultiples, Subtract>(a, primes);
                return;
            }
        }
        propagate_scalar<ToMultiples, Subtract>(a, primes);
    }
    template <bool ToMultiples, bool Subtract, class T, class PrimeRange>
    static void propagate_expanded(std::vector<T>& a, const PrimeRange& primes) {
        if (a.size() <= 2) return;
        const std::size_t upper = a.size() - 1;
        auto prime = primes.begin();
        const auto end = primes.end();
        for (; prime != end; ++prime) {
            const auto p = static_cast<std::size_t>(*prime);
            if (p > upper / 2) break;
            const auto count = upper / p;
            if constexpr (ToMultiples != Subtract) {
                std::size_t from = 1, to = p;
                for (; count - from + 1 >= 8; from += 8, to += p * 8) {
                    if constexpr (ToMultiples) a[to + p * 0] += a[from + 0]; else a[from + 0] -= a[to + p * 0];
                    if constexpr (ToMultiples) a[to + p * 1] += a[from + 1]; else a[from + 1] -= a[to + p * 1];
                    if constexpr (ToMultiples) a[to + p * 2] += a[from + 2]; else a[from + 2] -= a[to + p * 2];
                    if constexpr (ToMultiples) a[to + p * 3] += a[from + 3]; else a[from + 3] -= a[to + p * 3];
                    if constexpr (ToMultiples) a[to + p * 4] += a[from + 4]; else a[from + 4] -= a[to + p * 4];
                    if constexpr (ToMultiples) a[to + p * 5] += a[from + 5]; else a[from + 5] -= a[to + p * 5];
                    if constexpr (ToMultiples) a[to + p * 6] += a[from + 6]; else a[from + 6] -= a[to + p * 6];
                    if constexpr (ToMultiples) a[to + p * 7] += a[from + 7]; else a[from + 7] -= a[to + p * 7];
                }
                for (; from <= count; ++from, to += p) {
                    if constexpr (ToMultiples) a[to] += a[from]; else a[from] -= a[to];
                }
            } else {
                std::size_t from = count, to = count * p;
                for (; from >= 8; from -= 8, to -= p * 8) {
                    if constexpr (ToMultiples) a[to - p * 0] -= a[from - 0]; else a[from - 0] += a[to - p * 0];
                    if constexpr (ToMultiples) a[to - p * 1] -= a[from - 1]; else a[from - 1] += a[to - p * 1];
                    if constexpr (ToMultiples) a[to - p * 2] -= a[from - 2]; else a[from - 2] += a[to - p * 2];
                    if constexpr (ToMultiples) a[to - p * 3] -= a[from - 3]; else a[from - 3] += a[to - p * 3];
                    if constexpr (ToMultiples) a[to - p * 4] -= a[from - 4]; else a[from - 4] += a[to - p * 4];
                    if constexpr (ToMultiples) a[to - p * 5] -= a[from - 5]; else a[from - 5] += a[to - p * 5];
                    if constexpr (ToMultiples) a[to - p * 6] -= a[from - 6]; else a[from - 6] += a[to - p * 6];
                    if constexpr (ToMultiples) a[to - p * 7] -= a[from - 7]; else a[from - 7] += a[to - p * 7];
                }
                for (; from; --from, to -= p) {
                    if constexpr (ToMultiples) a[to] -= a[from]; else a[from] += a[to];
                }
            }
        }
        for (; prime != end; ++prime) {
            const auto p = static_cast<std::size_t>(*prime);
            if (p > upper) break;
            if constexpr (ToMultiples) {
                if constexpr (Subtract) a[p] -= a[1]; else a[p] += a[1];
            } else {
                if constexpr (Subtract) a[1] -= a[p]; else a[1] += a[p];
            }
        }
    }
#endif
    template <bool ToMultiples, bool Subtract, class T, class PrimeRange>
#if defined(__GNUC__) && !defined(__clang__)
    static void propagate_scalar(std::vector<T>& a, const PrimeRange& primes) {
#else
    static void propagate(std::vector<T>& a, const PrimeRange& primes) {
#endif
        if (a.size() <= 2) return;
        const std::size_t upper = a.size() - 1;
        auto prime = primes.begin();
        const auto end = primes.end();
        for (; prime != end; ++prime) {
            const auto p = static_cast<std::size_t>(*prime);
            if (p > upper / 2) break;
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
        for (; prime != end; ++prime) {
            const auto p = static_cast<std::size_t>(*prime);
            if (p > upper) break;
            if constexpr (ToMultiples) {
                if constexpr (Subtract) a[p] -= a[1]; else a[p] += a[1];
            } else {
                if constexpr (Subtract) a[1] -= a[p]; else a[1] += a[p];
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
    template <class T> static decltype(auto) product(const T& left, const T& right) {
        if constexpr (std::is_unsigned_v<T> && sizeof(T) < sizeof(unsigned int))
            return static_cast<T>(static_cast<unsigned int>(left) * static_cast<unsigned int>(right));
        else return left * right;
    }
    template <class T> static void multiply_assign(T& left, T& right) {
        if constexpr (std::is_unsigned_v<T> && sizeof(T) < sizeof(unsigned int))
            left = static_cast<T>(static_cast<unsigned int>(left) * static_cast<unsigned int>(right));
        else left *= right;
    }

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
                    mixed[k] = product(left_zero, b[k]) + product(a[k], right_zero);
            }
            reinsirk::detail::IntegerTransform::propagate<false, false>(a, primes);
            reinsirk::detail::IntegerTransform::propagate<false, false>(b, primes);
            for (std::size_t k = 0; k < a.size(); ++k) multiply_assign(a[k], b[k]);
            reinsirk::detail::IntegerTransform::propagate<false, true>(a, primes);
            if (zero_terms_needed)
                for (std::size_t k = 1; k < a.size(); ++k) a[k] += mixed[k];
        } else {
            T at_zero = product(left_zero, right_zero);
            if (zero_terms_needed) {
                T left_sum{}, right_sum{};
                for (std::size_t k = 1; k < a.size(); ++k) {
                    left_sum += a[k]; right_sum += b[k];
                }
                at_zero += product(left_zero, right_sum) + product(left_sum, right_zero);
            }
            reinsirk::detail::IntegerTransform::propagate<true, false>(a, primes);
            reinsirk::detail::IntegerTransform::propagate<true, false>(b, primes);
            for (std::size_t k = 1; k < a.size(); ++k) multiply_assign(a[k], b[k]);
            reinsirk::detail::IntegerTransform::propagate<true, true>(a, primes);
            a.front() = at_zero;
        }
        return a;
    }
    template <bool Gcd, class T, class PrimeRange>
    static std::vector<T> square(std::vector<T> a, const PrimeRange& primes) {
        if (a.empty()) return a;
        const T zero = a.front();
        bool zero_terms_needed = true;
        if constexpr (std::equality_comparable<T>) zero_terms_needed = zero != T{};
        if constexpr (Gcd) {
            std::vector<T> mixed;
            if (zero_terms_needed) {
                mixed.resize(a.size());
                for (std::size_t k = 1; k < a.size(); ++k)
                    mixed[k] = product(zero, a[k]) + product(a[k], zero);
            }
            IntegerTransform::propagate<false, false>(a, primes);
            for (std::size_t k = 0; k < a.size(); ++k) multiply_assign(a[k], a[k]);
            IntegerTransform::propagate<false, true>(a, primes);
            if (zero_terms_needed)
                for (std::size_t k = 1; k < a.size(); ++k) a[k] += mixed[k];
        } else {
            T at_zero = product(zero, zero);
            if (zero_terms_needed) {
                T sum{};
                for (std::size_t k = 1; k < a.size(); ++k) sum += a[k];
                at_zero += product(zero, sum) + product(sum, zero);
            }
            IntegerTransform::propagate<true, false>(a, primes);
            for (std::size_t k = 1; k < a.size(); ++k) multiply_assign(a[k], a[k]);
            IntegerTransform::propagate<true, true>(a, primes);
            a.front() = at_zero;
        }
        return a;
    }
};
} // namespace reinsirk::detail
#endif
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

#ifndef SIRKIT_GCD_SQUARE_DEFINED
#define SIRKIT_GCD_SQUARE_DEFINED
template <class T> std::vector<T> gcd_square(std::vector<T> f, const IntegerSieve& sieve) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_sieve(f.size(), sieve);
#endif
    return reinsirk::detail::IntegerConvolution::square<true>(std::move(f), sieve.prime_list);
}
template <class T> std::vector<T> gcd_square(std::vector<T> f) {
    std::vector<int> storage;
    const auto primes = reinsirk::detail::IntegerTransform::prime_view_for(f.size(), storage);
    return reinsirk::detail::IntegerConvolution::square<true>(std::move(f), primes);
}
#endif
#ifndef SIRKIT_LCM_SQUARE_DEFINED
#define SIRKIT_LCM_SQUARE_DEFINED
template <class T> std::vector<T> lcm_square(std::vector<T> f, const IntegerSieve& sieve) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_sieve(f.size(), sieve);
#endif
    return reinsirk::detail::IntegerConvolution::square<false>(std::move(f), sieve.prime_list);
}
template <class T> std::vector<T> lcm_square(std::vector<T> f) {
    std::vector<int> storage;
    const auto primes = reinsirk::detail::IntegerTransform::prime_view_for(f.size(), storage);
    return reinsirk::detail::IntegerConvolution::square<false>(std::move(f), primes);
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
