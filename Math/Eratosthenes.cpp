// SPDX-License-Identifier: CC0-1.0

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
// 内部用。4語ずつ数え、端の1〜3語だけ標準のpopcountを使う。
[[gnu::target("avx2"), gnu::noinline]] inline std::uint64_t popcount_words_avx2(
    const std::uint64_t* words, std::size_t count) noexcept {
    const auto table = _mm256_setr_epi8(
        0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
        0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4);
    const auto mask = _mm256_set1_epi8(15);
    const auto zero = _mm256_setzero_si256();
    auto sum = zero;
    std::size_t i = 0;
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

#ifndef SIRKIT_PRIME_SIEVE_DEFINED
#define SIRKIT_PRIME_SIEVE_DEFINED

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

#ifndef SIRKIT_ERATOSTHENES_DEFINED
#define SIRKIT_ERATOSTHENES_DEFINED
// eratosthenes(limit, ans): limit以下の素数を昇順でansの末尾へ追加する。O(limit log log limit)。
// eratosthenes(limit): limit以下の素数を昇順のvectorで返す。同じ計算量。
inline void eratosthenes(std::uint64_t limit, std::vector<std::uint64_t>& ans) {
    PrimeSieve(limit).append_to(ans);
}
inline std::vector<std::uint64_t> eratosthenes(std::uint64_t limit) {
    return PrimeSieve(limit).primes();
}
#endif
