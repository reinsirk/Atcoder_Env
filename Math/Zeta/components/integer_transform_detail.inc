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
