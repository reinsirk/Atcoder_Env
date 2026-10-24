// SPDX-License-Identifier: CC0-1.0
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
// [疎な倍数変換]
// m=f.size()、rは各keyの素因数を重複込みで数えた総数（以下の両API共通）。
// sparse_multiple_zeta_transform(f,sieve): 正で一意なkeyの全約数を含むfをkey昇順にし、各値を倍数和へ置換。key<=sieve.max_value()、Valは+=対応。O((m+r)log(m+1))時間、O(m+r)補助領域。
// sparse_multiple_mobius_transform(f,sieve): 正で一意なkeyの全約数を含むfをkey昇順にし、倍数和の逆変換を実行。key<=sieve.max_value()、Valは-=対応。O((m+r)log(m+1))時間、O(m+r)補助領域。

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
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

#ifndef SIRKIT_INTERNAL_SPARSE_INTEGER_TRANSFORM_DEFINED
#define SIRKIT_INTERNAL_SPARSE_INTEGER_TRANSFORM_DEFINED
namespace reinsirk::detail {
// 内部用。密な範囲は位置配列、疎な範囲は素数別の伝播列を使う。
template <bool Inverse, std::integral Int, class Val>
void sparse_integer_transform(std::vector<std::pair<Int, Val>>& f,
                                      const IntegerSieve& sieve) {
    if (f.empty()) return;
    // 1..mが順番に並ぶ場合は、添字を直接使い並べ替えと位置配列を省く。
    const std::size_t dense_size = f.size();
    if (f.front().first == Int{1}
        && static_cast<std::uintmax_t>(f.back().first) == dense_size) {
        bool consecutive = true;
        for (std::size_t i = 1; i + 1 < dense_size; ++i) {
            if (static_cast<std::uintmax_t>(f[i].first) != i + 1) {
                consecutive = false; break;
            }
        }
        if (consecutive) {
#ifndef SKIP_RUNTIME_CHECKS
            if (sieve.max_value() < 0 || dense_size > static_cast<std::uintmax_t>(sieve.max_value()))
                throw std::out_of_range("sparse integer transform: key out of range");
#endif
            auto prime = sieve.prime_list.begin();
            const auto end = sieve.prime_list.end();
            for (; prime != end; ++prime) {
                const auto p = static_cast<std::size_t>(*prime);
                if (p > dense_size / 2) break;
                const auto count = dense_size / p;
                if constexpr (Inverse) {
                    std::size_t from = 1, to = p;
#if defined(__GNUC__) && !defined(__clang__)
                    if constexpr (std::is_trivially_copyable_v<Val> && sizeof(Val) <= 8) {
                        for (; count - from + 1 >= 8; from += 8, to += p * 8) {
                            f[from - 1].second -= f[to - 1].second;
                            f[from].second -= f[to + p * 1 - 1].second;
                            f[from + 1].second -= f[to + p * 2 - 1].second;
                            f[from + 2].second -= f[to + p * 3 - 1].second;
                            f[from + 3].second -= f[to + p * 4 - 1].second;
                            f[from + 4].second -= f[to + p * 5 - 1].second;
                            f[from + 5].second -= f[to + p * 6 - 1].second;
                            f[from + 6].second -= f[to + p * 7 - 1].second;
                        }
                    }
#endif
                    for (; from <= count; ++from, to += p)
                        f[from - 1].second -= f[to - 1].second;
                } else {
                    std::size_t from = count, to = count * p;
#if defined(__GNUC__) && !defined(__clang__)
                    if constexpr (std::is_trivially_copyable_v<Val> && sizeof(Val) <= 8) {
                        for (; from >= 8; from -= 8, to -= p * 8) {
                            f[from - 1].second += f[to - 1].second;
                            f[from - 2].second += f[to - p * 1 - 1].second;
                            f[from - 3].second += f[to - p * 2 - 1].second;
                            f[from - 4].second += f[to - p * 3 - 1].second;
                            f[from - 5].second += f[to - p * 4 - 1].second;
                            f[from - 6].second += f[to - p * 5 - 1].second;
                            f[from - 7].second += f[to - p * 6 - 1].second;
                            f[from - 8].second += f[to - p * 7 - 1].second;
                        }
                    }
#endif
                    for (; from; --from, to -= p)
                        f[from - 1].second += f[to - 1].second;
                }
            }
            for (; prime != end; ++prime) {
                const auto p = static_cast<std::size_t>(*prime);
                if (p > dense_size) break;
                if constexpr (Inverse) f[0].second -= f[p - 1].second;
                else f[0].second += f[p - 1].second;
            }
            return;
        }
    }
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
template <std::integral Int, class Val>
void sparse_multiple_zeta_transform(std::vector<std::pair<Int, Val>>& f,
                              const IntegerSieve& sieve) {
    reinsirk::detail::sparse_integer_transform<false>(f, sieve);
}
#endif

#ifndef SIRKIT_SPARSE_FAST_INTEGER_MOEBIUS_DEFINED
#define SIRKIT_SPARSE_FAST_INTEGER_MOEBIUS_DEFINED
template <std::integral Int, class Val>
void sparse_multiple_mobius_transform(std::vector<std::pair<Int, Val>>& f,
                                 const IntegerSieve& sieve) {
    reinsirk::detail::sparse_integer_transform<true>(f, sieve);
}
#endif
