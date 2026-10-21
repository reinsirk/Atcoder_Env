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
