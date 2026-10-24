#define SKIP_RUNTIME_CHECKS
// Verify: https://judge.yosupo.jp/problem/enumerate_primes
// Submission: https://judge.yosupo.jp/submission/412343

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

int main() {
    std::uint64_t n,a,b;verify_in>>n>>a>>b;
    PrimeSieve sieve(n);const auto primes=sieve.primes();
    const auto size=primes.size();const auto x=size>b?(size-1-b)/a+1:0;
    verify_out<<sieve.prime_count()<<' '<<x<<'\n';
    for(std::uint64_t i=b;i<size;i+=a) verify_out<<primes[i]<<' ';
    verify_out<<'\n';
}
