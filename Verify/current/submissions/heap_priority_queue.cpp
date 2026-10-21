#define SKIP_RUNTIME_CHECKS
// Verify: https://atcoder.jp/contests/abc234/tasks/abc234_d

// Library: DataStructure/heap.cpp
// SPDX-License-Identifier: CC0-1.0

#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

#ifndef REINSIRK_HEAP_DEFINED
#define REINSIRK_HEAP_DEFINED

// int の最小ヒープ。値を所有し、コピー・ムーブ可能。
// Heap(): 空を構築。Heap(vector<int>): 値を受け取り O(N) で構築。
// Heap({値...}), Heap(first,last): 値をコピーし O(N) で構築。
// empty(), size(): 空判定・要素数を O(1) で返す。
// top(): 最小値、空なら -1 を O(1) で返す。top_or(fallback): 空なら fallback。
// push(value): 値を追加、償却 O(log N)。pop(): 最小値を削除、空なら何もしない、O(log N)。
// pop_value(): 最小値を削除して返す、O(log N)。空なら out_of_range、検査省略時は非空が前提。
// replace_top(value): 最小値を置換して旧値を返す、O(log N)。空の扱いは pop_value() と同じ。
// push_pop(value): 値を追加した後の最小値を返して削除、O(log N)、空なら value を返す。
// clear(): 全要素を削除、容量を保持。reserve(n): n 要素分の容量を確保、再確保時 O(N)。
class Heap {
    std::vector<int> values_;

    void make_heap() {
        std::make_heap(values_.begin(), values_.end(), std::greater<int>{});
    }

    void repair_root(int value) noexcept {
        const std::size_t count = values_.size();
        if (count == 1) {
            values_[0] = value;
            return;
        }
        std::size_t position;
        if (count > 2 && values_[2] < values_[1]) {
            if (value <= values_[2]) {
                values_[0] = value;
                return;
            }
            values_[0] = values_[2];
            position = 2;
        } else {
            if (value <= values_[1]) {
                values_[0] = value;
                return;
            }
            values_[0] = values_[1];
            position = 1;
        }
        while (position < count / 2) {
            const std::size_t left = position * 2 + 1;
            const std::size_t right = left + 1;
            if (right < count && values_[right] < values_[left]) {
                if (value <= values_[right]) break;
                values_[position] = values_[right];
                position = right;
            } else {
                if (value <= values_[left]) break;
                values_[position] = values_[left];
                position = left;
            }
        }
        values_[position] = value;
    }

public:
    Heap() = default;

    explicit Heap(std::vector<int> values) : values_(std::move(values)) {
        make_heap();
    }

    Heap(std::initializer_list<int> values) : values_(values) {
        make_heap();
    }

    template <std::input_iterator Iterator>
    Heap(Iterator first, Iterator last) : values_(first, last) {
        make_heap();
    }

    [[nodiscard]] bool empty() const noexcept { return values_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }

    [[nodiscard]] int top() const noexcept { return top_or(-1); }
    [[nodiscard]] int top_or(int fallback) const noexcept {
        return empty() ? fallback : values_.front();
    }

    void push(int value) {
        values_.push_back(value);
        std::size_t position = values_.size() - 1;
        while (position != 0) {
            const std::size_t parent = (position - 1) / 2;
            if (values_[parent] <= value) break;
            values_[position] = values_[parent];
            position = parent;
        }
        values_[position] = value;
    }

    void pop() noexcept {
        if (empty()) return;
        const int value = values_.back();
        values_.pop_back();
        if (!values_.empty()) repair_root(value);
    }

    int pop_value() {
#ifndef SKIP_RUNTIME_CHECKS
        if (empty()) throw std::out_of_range("Heap::pop_value: empty heap");
#endif
        const int result = values_.front();
        const int value = values_.back();
        values_.pop_back();
        if (!values_.empty()) repair_root(value);
        return result;
    }

    int replace_top(int value) {
#ifndef SKIP_RUNTIME_CHECKS
        if (empty()) throw std::out_of_range("Heap::replace_top: empty heap");
#endif
        const int result = values_.front();
        repair_root(value);
        return result;
    }

    int push_pop(int value) noexcept {
        if (empty() || value <= values_.front()) return value;
        const int result = values_.front();
        repair_root(value);
        return result;
    }

    void clear() noexcept { values_.clear(); }
    void reserve(std::size_t count) { values_.reserve(count); }
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


int main() {
    int n, k; verify_in >> n >> k;
    Heap heap;
    for (int i = 0; i < n; ++i) {
        int value; verify_in >> value;
        heap.push(value);
        if (heap.size() > static_cast<std::size_t>(k)) (void)heap.pop_value();
        if (i + 1 >= k) verify_out << heap.top() << '\n';
    }
}
