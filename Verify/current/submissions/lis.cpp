#define SKIP_RUNTIME_CHECKS
// Verify: https://judge.yosupo.jp/problem/longest_increasing_subsequence

// Library: Algorithm/LIS.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://zenn.dev/reputeless/books/standard-cpp-for-competitive-programming/viewer/lis
#include <algorithm>
#include <cstddef>
#include <limits>
#include <vector>

#ifndef SIRKIT_LIS_LENGTHS_DEFINED
#define SIRKIT_LIS_LENGTHS_DEFINED
// LIS<Strict>(v): 各接頭辞のLIS長を返す。trueは狭義、falseは広義。空入力は空。
// LISLength<Strict=true>(v): LIS長だけを返す。入力は変更せず、比較はoperator<を使う。
// 時間O(N log(L+1))、追加領域はLISがO(N+L)、LISLengthがO(L)。整列済みの増加列はO(N)。
namespace reinsirk::detail {
template <bool Strict, bool Prefix, class T>
auto LISLengths(const std::vector<T>& v) {
    std::vector<T> tails;
    tails.reserve(std::min(v.size(), std::size_t{64}));
    std::vector<std::size_t> lengths;
    if constexpr (Prefix) lengths.resize(v.size());
    std::size_t i = 0;
    for (const auto& value : v) {
        const bool extend = tails.empty() || (Strict ? tails.back() < value : !(value < tails.back()));
        if (extend) tails.push_back(value);
        else {
            if constexpr (Strict) *std::lower_bound(tails.begin(), tails.end(), value) = value;
            else *std::upper_bound(tails.begin(), tails.end(), value) = value;
        }
        if constexpr (Prefix) lengths[i++] = tails.size();
    }
    if constexpr (Prefix) return lengths;
    else return tails.size();
}
} // namespace reinsirk::detail
template <bool Strict, class T>
std::vector<std::size_t> LIS(const std::vector<T>& v) {
    return reinsirk::detail::LISLengths<Strict, true>(v);
}
template <bool Strict = true, class T>
std::size_t LISLength(const std::vector<T>& v) {
    return reinsirk::detail::LISLengths<Strict, false>(v);
}
#endif

#ifndef SIRKIT_LIS_RESTORE_DEFINED
#define SIRKIT_LIS_RESTORE_DEFINED
// LISIndices<Strict=true>(v): LISを1つ復元し、元配列の添字を昇順で返す。同長の解の選択は任意。
// LISSequence<Strict=true>(v): LISを1つ値の列としてコピーする。空なら空。いずれもO(N log(L+1))時間・O(N)追加領域。
template <bool Strict = true, class T>
std::vector<std::size_t> LISIndices(const std::vector<T>& v) {
    constexpr auto none = std::numeric_limits<std::size_t>::max();
    std::vector<std::size_t> tails, previous(v.size(), none);
    tails.reserve(std::min(v.size(), std::size_t{64}));
    for (std::size_t i = 0; i < v.size(); ++i) {
        std::size_t lo = 0, hi = tails.size();
        if (!tails.empty() && (Strict ? v[tails.back()] < v[i] : !(v[i] < v[tails.back()]))) lo = hi;
        else while (lo < hi) {
            const auto mid = lo + (hi - lo) / 2;
            const bool before = Strict ? v[tails[mid]] < v[i] : !(v[i] < v[tails[mid]]);
            if (before) lo = mid + 1;
            else hi = mid;
        }
        if (lo != 0) previous[i] = tails[lo - 1];
        if (lo == tails.size()) tails.push_back(i);
        else tails[lo] = i;
    }
    std::vector<std::size_t> result(tails.size());
    if (!tails.empty()) {
        auto p = tails.back();
        for (auto i = result.size(); i != 0; --i) {
            result[i - 1] = p;
            p = previous[p];
        }
    }
    return result;
}
template <bool Strict = true, class T>
std::vector<T> LISSequence(const std::vector<T>& v) {
    const auto indices = LISIndices<Strict>(v);
    std::vector<T> result;
    result.reserve(indices.size());
    for (auto i : indices) result.push_back(v[i]);
    return result;
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

#include <cstdint>
#include <vector>
int main(){
    int n{};verify_in>>n;std::vector<std::int64_t>a(n);for(auto&x:a)verify_in>>x;
    const auto answer=LISIndices<true>(a);verify_out<<answer.size()<<'\n';
    for(std::size_t i=0;i<answer.size();++i)verify_out<<(i?" ":"")<<answer[i];
    verify_out<<'\n';
}
