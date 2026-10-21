#define SKIP_RUNTIME_CHECKS
// Verify: https://judge.yosupo.jp/problem/zalgorithm

// Library: String/RollingHash.cpp
// SPDX-License-Identifier: CC0-1.0

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#ifndef SIRKIT_ROLLING_HASH_DEFINED
#define SIRKIT_ROLLING_HASH_DEFINED

// RollingHash(s, base): 符号なしバイト列をO(|s|)時間・空間で構築。base<2は10、base>modulus-2はmodulus-10に補正。
// modulus: ハッシュの法2^61-1。ハッシュ値は常に[0,modulus)。
// get(l,r): 半開区間[l,r)のハッシュ、whole(): 全体のハッシュをO(1)で返す。空区間は0。
// size(), empty(), base(): 長さ・空判定・補正後の底をO(1)で返す。
// concat(left,right,right_length): 同じ底の正規化済みハッシュを連結してO(1)で返す。right_length<=size()。
// equal(l,r,other,ol,orr): 同じ底の2区間の長さとハッシュをO(1)で比較する。
// lcp(l,other,ol,limit): 同じ底の接尾辞の最長共通接頭辞長をlimit以下でO(log(min(残長,limit)+1))で返す。
// compare(l,r,other,ol,orr): 同じ底の2区間を符号なしバイト辞書順で比較し-1/0/1をO(log(短い方の長さ+1))で返す。
// 比較にはハッシュ衝突による誤判定の可能性がある。検査有効時は不正な範囲にout_of_range、底・ハッシュにinvalid_argument。
// コピーは独立し代入は強い例外保証。ムーブ元は空（自身へのムーブ代入は保持）。swapはO(1)。
class RollingHash {
public:
    static constexpr std::uint64_t modulus = (std::uint64_t{1} << 61) - 1;

    RollingHash(std::string_view s, std::uint64_t base)
        : base_(normalize_base(base)) {
        if (s.size() >= data_.max_size() / 2) {
            throw std::length_error("RollingHash: capacity overflow");
        }
        data_.resize(s.size() + 1);
        power_data_.resize(s.size() + 1);
        const std::size_t offset = s.size() + 1;
        offset_ = offset;
        Entry* const prefixes = data_.data();
        Entry* const powers = power_data_.data();
        powers_ = powers;
        const std::uint64_t factor = base_;
        std::uint64_t hash = 0;
        std::uint64_t power = 1;
        prefixes[0].value = hash;
        powers[0].value = power;
        for (std::size_t i = 0; i < s.size(); ++i) {
            const auto byte = static_cast<unsigned char>(s[i]);
            hash = multiply_add_byte(hash, factor, byte);
            power = multiply(power, factor);
            prefixes[i + 1].value = hash;
            powers[i + 1].value = power;
        }
    }

    RollingHash(const RollingHash& other)
        : data_(other.data_), power_data_(other.power_data_),
          base_(other.base_), offset_(other.offset_),
          powers_(data_.empty() ? nullptr : power_data_.data()) {}

    RollingHash& operator=(const RollingHash& other) {
        if (this != &other) {
            RollingHash copy(other);
            swap(copy);
        }
        return *this;
    }

    RollingHash(RollingHash&& other) noexcept
        : data_(std::move(other.data_)), power_data_(std::move(other.power_data_)),
          base_(other.base_), offset_(other.offset_),
          powers_(other.powers_) {
        other.data_.clear();
        other.power_data_.clear();
        other.offset_ = 0;
        other.powers_ = nullptr;
    }

    RollingHash& operator=(RollingHash&& other) noexcept {
        if (this != &other) {
            data_ = std::move(other.data_);
            power_data_ = std::move(other.power_data_);
            base_ = other.base_;
            offset_ = other.offset_;
            powers_ = other.powers_;
            other.data_.clear();
            other.power_data_.clear();
            other.offset_ = 0;
            other.powers_ = nullptr;
        }
        return *this;
    }

    void swap(RollingHash& other) noexcept {
        data_.swap(other.data_);
        power_data_.swap(other.power_data_);
        std::swap(base_, other.base_);
        std::swap(offset_, other.offset_);
        std::swap(powers_, other.powers_);
    }

    friend void swap(RollingHash& left, RollingHash& right) noexcept {
        left.swap(right);
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return offset_ == 0 ? 0 : offset_ - 1;
    }

    [[nodiscard]] bool empty() const noexcept {
        return size() == 0;
    }

    [[nodiscard]] std::uint64_t base() const noexcept {
        return base_;
    }

    [[nodiscard]] std::uint64_t whole() const noexcept {
        return offset_ == 0 ? 0 : data_[offset_ - 1].value;
    }

    [[nodiscard]] std::uint64_t get(std::size_t l, std::size_t r) const {
#ifndef SKIP_RUNTIME_CHECKS
        check_range(l, r);
#endif
        return get_unchecked(l, r);
    }

    [[nodiscard]] std::uint64_t concat(std::uint64_t left,
                                     std::uint64_t right,
                                     std::size_t right_length) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (right_length > size()) {
            throw std::out_of_range("RollingHash: concatenation length");
        }
        if (left >= modulus || right >= modulus) {
            throw std::invalid_argument("RollingHash: noncanonical hash");
        }
#endif
        return add(right_length == 0 ? left
                                    : multiply(left, powers_[right_length].value),
                   right);
    }

    [[nodiscard]] bool equal(std::size_t l, std::size_t r,
                             const RollingHash& other,
                             std::size_t ol, std::size_t orr) const {
#ifndef SKIP_RUNTIME_CHECKS
        check_range(l, r);
        other.check_range(ol, orr);
        check_base(other);
#endif
        return r - l == orr - ol &&
               get_unchecked(l, r) == other.get_unchecked(ol, orr);
    }

    [[nodiscard]] std::size_t lcp(
        std::size_t l, const RollingHash& other, std::size_t ol,
        std::size_t limit = std::numeric_limits<std::size_t>::max()) const {
#ifndef SKIP_RUNTIME_CHECKS
        check_range(l, l);
        other.check_range(ol, ol);
        check_base(other);
#endif
        return lcp_unchecked(l, other, ol,
                             std::min({size() - l, other.size() - ol, limit}));
    }

    [[nodiscard]] int compare(std::size_t l, std::size_t r,
                              const RollingHash& other,
                              std::size_t ol, std::size_t orr) const {
#ifndef SKIP_RUNTIME_CHECKS
        check_range(l, r);
        other.check_range(ol, orr);
        check_base(other);
#endif
        const std::size_t length = r - l;
        const std::size_t other_length = orr - ol;
        const std::size_t common = lcp_unchecked(
            l, other, ol, std::min(length, other_length));
        if (common == std::min(length, other_length)) {
            return (length > other_length) - (length < other_length);
        }
        const std::uint64_t byte = get_unchecked(l + common, l + common + 1);
        const std::uint64_t other_byte =
            other.get_unchecked(ol + common, ol + common + 1);
        return (byte > other_byte) - (byte < other_byte);
    }

private:
    struct Entry {
        std::uint64_t value;
        // 構築ループで全要素を設定し、不要なゼロ初期化を避ける。
        Entry() noexcept {}
    };

    std::vector<Entry> data_, power_data_;
    std::uint64_t base_;
    std::size_t offset_ = 0;
    const Entry* powers_ = nullptr; // power_data_の累乗領域への非所有ポインタ。

    static constexpr std::uint64_t normalize_base(std::uint64_t base) noexcept {
        return base < 2 ? 10 : (base > modulus - 2 ? modulus - 10 : base);
    }

    static constexpr std::uint64_t add(std::uint64_t a,
                                       std::uint64_t b) noexcept {
        const std::uint64_t sum = a + b;
        return sum >= modulus ? sum - modulus : sum;
    }

    static constexpr std::uint64_t multiply_raw(std::uint64_t a,
                                            std::uint64_t b) noexcept {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && \
    !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
        const __uint128_t product = static_cast<__uint128_t>(a) * b;
        const std::uint64_t folded =
            (static_cast<std::uint64_t>(product) & modulus) +
            static_cast<std::uint64_t>(product >> 61);
#else
        // 32bitの4積を2^61=1、2^64=8として直接折り返す。
        const std::uint64_t a0 = static_cast<std::uint32_t>(a);
        const std::uint64_t b0 = static_cast<std::uint32_t>(b);
        const std::uint64_t a1 = a >> 32;
        const std::uint64_t b1 = b >> 32;
        const std::uint64_t low = a0 * b0;
        const std::uint64_t cross = a0 * b1 + a1 * b0;
        constexpr std::uint64_t mask29 = (std::uint64_t{1} << 29) - 1;
        const std::uint64_t sum = (low & modulus) + (low >> 61) +
                                  ((cross & mask29) << 32) + (cross >> 29) +
                                  ((a1 * b1) << 3);
        const std::uint64_t folded = sum;
#endif
        return folded;
    }

    static constexpr std::uint64_t multiply(std::uint64_t a, std::uint64_t b) noexcept {
        std::uint64_t folded = multiply_raw(a, b);
#if !((defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__)) || defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
        folded = (folded & modulus) + (folded >> 61);
#endif
        return folded >= modulus ? folded - modulus : folded;
    }

    // byteは0..255。乗算結果への加算を正規化前にまとめる。
    static constexpr std::uint64_t multiply_add_byte(
        std::uint64_t a, std::uint64_t b, unsigned char byte) noexcept {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
        const __uint128_t product = static_cast<__uint128_t>(a) * b + byte;
        const std::uint64_t folded =
            (static_cast<std::uint64_t>(product) & modulus) +
            static_cast<std::uint64_t>(product >> 61);
#else
        const std::uint64_t sum = multiply_raw(a, b) + byte;
        const std::uint64_t folded = (sum & modulus) + (sum >> 61);
#endif
        return folded >= modulus ? folded - modulus : folded;
    }

    [[nodiscard]] std::uint64_t get_unchecked(std::size_t l,
                                             std::size_t r) const noexcept {
        if (l == r) {
            return 0;
        }
        const std::uint64_t right = data_[r].value;
        const std::uint64_t left = multiply_raw(data_[l].value, powers_[r - l].value);
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
        const std::uint64_t difference = right + 2 * modulus - left;
#else
        // raw積は4p未満、右辺全体は5p未満で64bitに収まる。
        const std::uint64_t difference = right + 4 * modulus - left;
#endif
        const std::uint64_t folded = (difference & modulus) + (difference >> 61);
        return folded >= modulus ? folded - modulus : folded;
    }

    [[nodiscard]] std::size_t lcp_unchecked(std::size_t l,
                                           const RollingHash& other,
                                           std::size_t ol,
                                           std::size_t limit) const noexcept {
        std::size_t low = 0;
        std::size_t high = limit;
        while (low < high) {
            const std::size_t middle = high - (high - low) / 2;
            if (get_unchecked(l, l + middle) ==
                other.get_unchecked(ol, ol + middle)) {
                low = middle;
            } else {
                high = middle - 1;
            }
        }
        return low;
    }

#ifndef SKIP_RUNTIME_CHECKS
    void check_range(std::size_t l, std::size_t r) const {
        if (l > r || r > size()) {
            throw std::out_of_range("RollingHash: substring range");
        }
    }

    void check_base(const RollingHash& other) const {
        if (base_ != other.base_) {
            throw std::invalid_argument("RollingHash: different bases");
        }
    }
#endif
};

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

#include <cstddef>
#include <cstdint>
#include <string>

int main() {
    std::string text; verify_in >> text;
    RollingHash hash(text, std::uint64_t{1000000007});
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (i) verify_out << ' ';
        verify_out << hash.lcp(0, hash, i);
    }
    verify_out << '\n';
}
