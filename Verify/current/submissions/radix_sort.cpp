#define SKIP_RUNTIME_CHECKS
// Verify: https://atcoder.jp/contests/abc331/tasks/abc331_c

// Library: Algorithm/Radix_sort.cpp
// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef REINSIRK_RADIX_SORT_DETAIL_DEFINED
#define REINSIRK_RADIX_SORT_DETAIL_DEFINED
namespace reinsirk::detail {
template <class T>
concept RadixInteger = std::integral<T> && !std::same_as<T, bool> &&
    !std::is_const_v<T> && !std::is_volatile_v<T> &&
    (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8) &&
    std::numeric_limits<std::make_unsigned_t<T>>::digits == sizeof(T) * 8;

template <RadixInteger T> struct RadixSortDetail {
    using Unsigned = std::make_unsigned_t<T>;
    static constexpr std::size_t bytes = sizeof(T);
    static constexpr std::size_t small_limit = 64;
    using Histogram = std::array<std::array<std::size_t, 256>, bytes>;

    static Unsigned key(T value) noexcept {
        Unsigned result = static_cast<Unsigned>(value);
        if constexpr (std::is_signed_v<T>) result ^= Unsigned{1} << (bytes * 8 - 1);
        return result;
    }
    template <bool HasMaximum> static void check(T value, [[maybe_unused]] T maximum) {
#ifndef SKIP_RUNTIME_CHECKS
        if constexpr (HasMaximum) {
            if (value > maximum) throw std::out_of_range("RadixSort: value exceeds maximum");
        }
#else
        (void)value;
#endif
    }
    template <bool HasMaximum>
    static Unsigned histogram(const std::vector<T>& values, Histogram& counts, T maximum, bool& ordered, bool& reversed) {
        const Unsigned first = key(values.front());
        Unsigned different = 0;
        T previous = values.front();
        ordered = reversed = true;
        for (T value : values) {
            check<HasMaximum>(value, maximum);
            ordered &= previous <= value;
            reversed &= previous >= value;
            previous = value;
            Unsigned encoded = key(value);
            different |= encoded ^ first;
            for (std::size_t byte = 0; byte < bytes; ++byte) {
                ++counts[byte][encoded & Unsigned{255}];
                if constexpr (bytes > 1) encoded >>= 8;
            }
        }
        return different;
    }
    static void prefix(std::array<std::size_t, 256>& counts) noexcept {
        std::size_t offset = 0;
        for (auto& count : counts) {
            const auto current = count;
            count = offset;
            offset += current;
        }
    }
    template <bool HasMaximum>
    static void sort(std::vector<T>& values, std::vector<T>& buffer, T maximum) {
        const std::size_t n = values.size();
        if (n <= small_limit) {
            if (n) check<HasMaximum>(values[0], maximum);
            for (std::size_t i = 1; i < n; ++i) {
                const T value = values[i];
                check<HasMaximum>(value, maximum);
                std::size_t j = i;
                for (; j && value < values[j - 1]; --j) values[j] = values[j - 1];
                values[j] = value;
            }
            return;
        }
        Histogram counts{};
        bool ordered, reversed;
        const Unsigned different = histogram<HasMaximum>(values, counts, maximum, ordered, reversed);
        if (ordered) return;
        if (reversed) {
            std::reverse(values.begin(), values.end());
            return;
        }
        buffer.resize(n);
        T* source = values.data();
        T* destination = buffer.data();
        for (std::size_t byte = 0; byte < bytes; ++byte) {
            const std::size_t shift = byte * 8;
            if (((different >> shift) & Unsigned{255}) == 0) continue;
            auto& offsets = counts[byte];
            prefix(offsets);
            for (std::size_t i = 0; i < n; ++i) {
                const T value = source[i];
                const auto bucket = static_cast<std::size_t>((key(value) >> shift) & Unsigned{255});
                destination[offsets[bucket]++] = value;
            }
            std::swap(source, destination);
        }
        if (source != values.data()) std::copy_n(source, n, values.data());
    }
};
} // namespace reinsirk::detail
#endif

#ifndef REINSIRK_RADIX_SORT_WORKSPACE_DEFINED
#define REINSIRK_RADIX_SORT_WORKSPACE_DEFINED
// RadixSortWorkspace<T>(): boolを除く8/16/32/64bit整数用の再利用作業領域。
// sort(values): 符号付きも全域で安定昇順ソート。B=sizeof(T)としてO(Bn)、追加領域O(n)。
// sort(values,maximum): 同じ処理。全要素<=maximumが前提で、通常処理中に検査する。
// reserve(n): n要素分の作業領域を予約。capacity(): 現在の予約要素数。
// release(): 作業領域を解放する。sortはvaluesの容量・要素の保管先を変えない。
template <reinsirk::detail::RadixInteger T> class RadixSortWorkspace {
    std::vector<T> buffer_;
public:
    void sort(std::vector<T>& values) {
        reinsirk::detail::RadixSortDetail<T>::template sort<false>(values, buffer_, T{});
    }
    void sort(std::vector<T>& values, T maximum) {
        reinsirk::detail::RadixSortDetail<T>::template sort<true>(values, buffer_, maximum);
    }
    void reserve(std::size_t count) { buffer_.reserve(count); }
    std::size_t capacity() const noexcept { return buffer_.capacity(); }
    void release() noexcept { std::vector<T>().swap(buffer_); }
};
#endif

#ifndef REINSIRK_RADIX_SORT_DEFINED
#define REINSIRK_RADIX_SORT_DEFINED
// RadixSort(values): 符号付きも全域で安定昇順ソート、O(sizeof(T)*n)時間・O(n)追加領域。
// RadixSort(values,maximum): 同じ処理。全要素<=maximumが前提（負の値・負のmaximumも可）。
// 小配列は確保せず挿入ソート。整列済みは変更せず、一定のbyteは散布を省く。
// valuesの容量・要素の保管先は不変。型Tはboolを除く8/16/32/64bit整数。
template <reinsirk::detail::RadixInteger T> void RadixSort(std::vector<T>& values) {
    RadixSortWorkspace<T> workspace;
    workspace.sort(values);
}
template <reinsirk::detail::RadixInteger T> void RadixSort(std::vector<T>& values, T maximum) {
    RadixSortWorkspace<T> workspace;
    workspace.sort(values, maximum);
}
#endif

#ifndef REINSIRK_RADIX_SORT_ORDER_DEFINED
#define REINSIRK_RADIX_SORT_ORDER_DEFINED
// RadixSortOrder(values): 昇順の元添字列を返す。同値は元の順、入力不変。
// boolを除く8/16/32/64bit整数。O(sizeof(T)*n)時間・O(n)追加領域。
template <reinsirk::detail::RadixInteger T> std::vector<std::size_t> RadixSortOrder(const std::vector<T>& values) {
    using Detail = reinsirk::detail::RadixSortDetail<T>;
    using Unsigned = typename Detail::Unsigned;
    const std::size_t n = values.size();
    std::vector<std::size_t> order(n);
    std::iota(order.begin(), order.end(), std::size_t{0});
    if (n <= Detail::small_limit) {
        for (std::size_t i = 1; i < n; ++i) {
            const std::size_t index = order[i];
            std::size_t j = i;
            for (; j && values[index] < values[order[j - 1]]; --j) order[j] = order[j - 1];
            order[j] = index;
        }
        return order;
    }
    typename Detail::Histogram counts{};
    bool ordered, reversed;
    const Unsigned different = Detail::template histogram<false>(values, counts, T{}, ordered, reversed);
    if (ordered) return order;
    if (reversed) {
        // 同じ値の区間内は元の添字順を保つ。
        std::size_t output = 0, end = n;
        while (end) {
            std::size_t begin = end - 1;
            while (begin && values[begin - 1] == values[end - 1]) --begin;
            for (std::size_t i = begin; i < end; ++i) order[output++] = i;
            end = begin;
        }
        return order;
    }
    std::vector<std::size_t> buffer(n);
    for (std::size_t byte = 0; byte < Detail::bytes; ++byte) {
        const std::size_t shift = byte * 8;
        if (((different >> shift) & Unsigned{255}) == 0) continue;
        auto& offsets = counts[byte];
        Detail::prefix(offsets);
        for (std::size_t index : order) {
            const auto bucket = static_cast<std::size_t>((Detail::key(values[index]) >> shift) & Unsigned{255});
            buffer[offsets[bucket]++] = index;
        }
        order.swap(buffer);
    }
    return order;
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

#ifndef REINSIRK_VERIFY_FAST_IO_DEFINED
#define REINSIRK_VERIFY_FAST_IO_DEFINED

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
#include <cstddef>
#include <cstdint>
#include <vector>

int main() {
    int n{}; verify_in >> n;
    std::vector<std::int64_t> values(n);
    for (auto& value : values) verify_in >> value;
    auto sorted = values;
    RadixSort(sorted);
    std::vector<std::int64_t> prefix(n + 1);
    for (int i = 0; i < n; ++i) prefix[i + 1] = prefix[i] + sorted[i];
    for (int i = 0; i < n; ++i) {
        const auto position = std::upper_bound(sorted.begin(), sorted.end(), values[i]) - sorted.begin();
        if (i) verify_out << ' ';
        verify_out << prefix[n] - prefix[static_cast<std::size_t>(position)];
    }
    verify_out << '\n';
}
