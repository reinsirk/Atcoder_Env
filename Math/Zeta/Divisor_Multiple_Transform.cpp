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
// [約数／倍数のゼータ／メビウス変換]
// 正の添字の4変換。f[0]は不変、空配列も可。時間O(n log log n)。
// multiple_zeta_transform(f): f[d]=sum_{d|k} f[k]へ更新する。
// multiple_mobius_transform(f): 倍数ゼータ変換を元に戻す。
// divisor_zeta_transform(f): f[k]=sum_{d|k} f[d]へ更新する。
// divisor_mobius_transform(f): 約数ゼータ変換を元に戻す。
// 各(f,sieve)版: sieve.max_value()>=f.size()-1の素数表を再利用し、追加領域O(1)。
// 各(f)版: 素数表も作る。f.size()-1<=INT_MAX、追加領域O(n)。

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

