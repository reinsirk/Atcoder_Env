#define SKIP_RUNTIME_CHECKS
// Verify: https://judge.yosupo.jp/problem/double_ended_priority_queue
// Submission: https://judge.yosupo.jp/submission/412316

// Library: DataStructure/erasable-priority-queue.cpp
// SPDX-License-Identifier: CC0-1.0
// Original Source: https://nyaannyaan.github.io/library/data-structure/erasable-priority-queue.hpp
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/erasable_priority_queue.md
//
// API一覧
// [ErasablePriorityQueue<T,Minimize>]
// ErasablePriorityQueue<T, Minimize>: trueなら最小値、falseなら最大値を先頭にする。
// ErasablePriorityQueue(values): vectorを受け取りO(n)で一括構築する。
// push(value), push(T&&): 1要素を追加する。O(log H)。
// emplace(args...): 引数から要素を構築して追加する。O(log H)。
// top(): 先頭の値を返す。空でないこと。遅延削除を含め償却O(log H)。
// pop(): 先頭を1個削除する。空でないこと。償却O(log H)。
// erase(value): 存在する値を1個遅延削除する。存在・個数は検査しない。O(log H)。
// empty(), size(): 有効な要素の有無・個数を返す。const、O(1)。
// try_top(): 空ならnullopt、そうでなければ先頭の値を返す。償却O(log H)。
// extract_top(): 空ならnullopt、そうでなければ先頭を1個削除して返す。償却O(log H)。
// swap(other): 有効な要素と遅延削除をまとめて交換する。O(1)。
// clear(): 全要素と遅延削除を破棄する。O(H)。Hは未回収の削除済み要素も含む保存数。
// Tの比較は厳密弱順序を満たし、==はその同値関係と一致すること。

#include <cstddef>
#include <functional>
#include <optional>
#include <queue>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef SIRKIT_ERASABLE_PRIORITY_QUEUE_DEFINED
#define SIRKIT_ERASABLE_PRIORITY_QUEUE_DEFINED

template <typename T, bool Minimize = true>
class ErasablePriorityQueue {
public:
    using Queue = std::conditional_t<Minimize,
        std::priority_queue<T, std::vector<T>, std::greater<T>>,
        std::priority_queue<T>>;

    ErasablePriorityQueue() = default;
    explicit ErasablePriorityQueue(std::vector<T> values)
        : values_(typename Queue::value_compare{}, std::move(values)) {}

    ErasablePriorityQueue(const ErasablePriorityQueue&) = default;
    ErasablePriorityQueue& operator=(const ErasablePriorityQueue&) = default;
    ErasablePriorityQueue(ErasablePriorityQueue&& other) noexcept {
        values_.swap(other.values_);
        erased_.swap(other.erased_);
    }
    ErasablePriorityQueue& operator=(ErasablePriorityQueue&& other) noexcept {
        if (this != &other) {
            ErasablePriorityQueue next(std::move(other));
            values_.swap(next.values_);
            erased_.swap(next.erased_);
        }
        return *this;
    }

    void push(const T& value) { values_.push(value); }
    void push(T&& value) { values_.push(std::move(value)); }
    template <class... Args>
    void emplace(Args&&... args) { values_.emplace(std::forward<Args>(args)...); }

    T top() {
#if defined(__GNUC__) && !defined(__clang__)
        if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
            if (!erased_.empty()) normalize();
        } else normalize();
#else
        normalize();
#endif
#ifndef SKIP_RUNTIME_CHECKS
        if (values_.empty()) [[unlikely]]
            throw std::out_of_range("ErasablePriorityQueue::top: empty queue");
#endif
        return values_.top();
    }
    void pop() {
        normalize();
#ifndef SKIP_RUNTIME_CHECKS
        if (values_.empty()) [[unlikely]]
            throw std::out_of_range("ErasablePriorityQueue::pop: empty queue");
#endif
        values_.pop();
    }
    void erase(const T& value) {
#ifndef SKIP_RUNTIME_CHECKS
        if (empty()) [[unlikely]]
            throw std::out_of_range("ErasablePriorityQueue::erase: empty queue");
#endif
        erased_.push(value);
    }
    bool empty() const noexcept { return values_.size() == erased_.size(); }
    std::size_t size() const noexcept { return values_.size() - erased_.size(); }

    std::optional<T> try_top() {
        if (empty()) return std::nullopt;
        normalize();
        return values_.top();
    }
    std::optional<T> extract_top() {
        if (empty()) return std::nullopt;
        normalize();
        std::optional<T> result(values_.top());
        values_.pop();
        return result;
    }
    void swap(ErasablePriorityQueue& other) noexcept {
        values_.swap(other.values_);
        erased_.swap(other.erased_);
    }
    void clear() noexcept {
        Queue{}.swap(values_);
        Queue{}.swap(erased_);
    }

private:
    struct Heap : Queue {
        using Queue::Queue;
        auto& storage() noexcept { return this->c; }
#if defined(__GNUC__) && !defined(__clang__)
        void pop() {
            auto& values = this->c;
            if (values.size() == 1) {
                values.pop_back();
                return;
            }
            const T value = values.back();
            values.pop_back();
            auto* const data = values.data();
            const auto count = values.size();
            std::size_t hole = 0;
            const auto cmp = typename Queue::value_compare{};
            while (hole < count / 2) {
                auto child = hole * 2 + 1;
                if (child + 1 < count && cmp(data[child], data[child + 1])) ++child;
                data[hole] = data[child];
                hole = child;
            }
            while (hole != 0) {
                const auto parent = (hole - 1) / 2;
                if (!cmp(data[parent], value)) break;
                data[hole] = data[parent];
                hole = parent;
            }
            data[hole] = value;
        }
#endif
    };
    using Storage = std::conditional_t<std::is_integral_v<T> && !std::is_same_v<T, bool>,
        Heap, Queue>;
    Storage values_, erased_;

    void pop_pair() {
        if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
            auto& a = values_.storage();
            auto& b = erased_.storage();
            const T last_a = a.back(), last_b = b.back();
            a.pop_back();
            b.pop_back();
            T* const pa = a.data();
            T* const pb = b.data();
            const std::size_t na = a.size(), nb = b.size();
            std::size_t ha = 0, hb = 0;
            const auto cmp = typename Queue::value_compare{};
            const auto descend = [&cmp](T* data, std::size_t count, std::size_t& hole) {
                std::size_t child = hole * 2 + 1;
                if (child + 1 < count && cmp(data[child], data[child + 1])) ++child;
                data[hole] = data[child];
                hole = child;
            };
            // 二つの独立した二分ヒープを交互に降りる。
            while (ha < na / 2 && hb < nb / 2) {
                descend(pa, na, ha);
                descend(pb, nb, hb);
            }
            while (ha < na / 2) descend(pa, na, ha);
            while (hb < nb / 2) descend(pb, nb, hb);
            const auto restore = [&cmp](T* data, std::size_t hole, const T value) {
                while (hole != 0) {
                    const std::size_t parent = (hole - 1) / 2;
                    if (!cmp(data[parent], value)) break;
                    data[hole] = data[parent];
                    hole = parent;
                }
                data[hole] = value;
            };
            if (na != 0) restore(pa, ha, last_a);
            if (nb != 0) restore(pb, hb, last_b);
        } else {
            values_.pop();
            erased_.pop();
        }
    }

    void normalize() {
        while (!erased_.empty() && values_.top() == erased_.top()) {
            if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
                pop_pair();
            } else {
                values_.pop();
                erased_.pop();
            }
        }
    }
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

#include <cstdint>

int main() {
    int n{}, q{}; if (!(verify_in >> n >> q)) return 0;
    ErasablePriorityQueue<std::int64_t> low;
    ErasablePriorityQueue<std::int64_t, false> high;
    auto add = [&](std::int64_t x) { low.push(x); high.push(x); };
    while (n--) { std::int64_t x{}; verify_in >> x; add(x); }
    while (q--) {
        int t{}; verify_in >> t;
        if (t == 0) { std::int64_t x{}; verify_in >> x; add(x); }
        else if (t == 1) {
            auto x = low.top(); low.pop(); high.erase(x); verify_out << x << '\n';
        } else {
            auto x = high.top(); high.pop(); low.erase(x); verify_out << x << '\n';
        }
    }
}
