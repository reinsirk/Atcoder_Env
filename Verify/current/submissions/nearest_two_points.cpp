#define SKIP_RUNTIME_CHECKS
// Verify: https://judge.yosupo.jp/problem/closest_pair

// Library: Geometry/Nearest_Two_points.cpp
// SPDX-License-Identifier: CC0-1.0

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef SIRKIT_NEAREST_TWO_POINTS_TYPES_DEFINED
#define SIRKIT_NEAREST_TWO_POINTS_TYPES_DEFINED
using P = std::pair<std::int32_t, std::int32_t>;

// distance_squaredは最短距離の二乗、添字は元入力の0始まり（first_index < second_index）。
struct NearestTwoPointsResult {
    std::uint64_t distance_squared;
    std::size_t first_index;
    std::size_t second_index;
};
#endif

#ifndef SIRKIT_INTERNAL_NEAREST_TWO_POINTS_DEFINED
#define SIRKIT_INTERNAL_NEAREST_TWO_POINTS_DEFINED
namespace reinsirk::detail {
class NearestTwoPointsSolver {
    struct Node {
        std::int32_t x;
        std::int32_t y;
        std::size_t index;
    };
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && \
    !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
    // GCC/Clangの整数拡張。標準C++の型ではない。
    using Distance = unsigned __int128;
    static Distance square(std::uint64_t d) { return Distance(d) * d; }
    static Distance add(Distance a, Distance b) { return a + b; }
    static std::uint64_t narrow(Distance d) {
        if (d > std::numeric_limits<std::uint64_t>::max())
            throw std::overflow_error("nearest_two_points: squared distance exceeds uint64_t");
        return static_cast<std::uint64_t>(d);
    }
#else
    // 座標差は32ビット以内。二乗和だけ65ビット必要になる。
    struct Distance {
        std::uint64_t low;
        bool high;
        friend bool operator<(Distance a, Distance b) {
            return a.high != b.high ? !a.high : a.low < b.low;
        }
    };
    static Distance square(std::uint64_t d) { return {d * d, false}; }
    static Distance add(Distance a, Distance b) {
        const auto low = a.low + b.low;
        return {low, low < a.low};
    }
    static std::uint64_t narrow(Distance d) {
        if (d.high)
            throw std::overflow_error("nearest_two_points: squared distance exceeds uint64_t");
        return d.low;
    }
#endif
    std::vector<Node> nodes_;
    std::vector<Node> buffer_;
    Distance best_{};
    std::size_t best_first_ = 0;
    std::size_t best_second_ = 1;
    bool zero_ = false;

#ifndef SKIP_RUNTIME_CHECKS
    template<std::integral Integer>
    static bool valid_coordinate(Integer value) {
        if constexpr (std::numeric_limits<Integer>::digits <= 31) {
            return true;
        } else if constexpr (std::is_signed_v<Integer>) {
            return value >= static_cast<Integer>(std::numeric_limits<std::int32_t>::min()) &&
                   value <= static_cast<Integer>(std::numeric_limits<std::int32_t>::max());
        } else {
            return value <= static_cast<Integer>(std::numeric_limits<std::int32_t>::max());
        }
    }
#endif
    static std::uint64_t difference(std::int32_t a, std::int32_t b) {
        const std::int64_t d = std::int64_t(a) - b;
        return static_cast<std::uint64_t>(d < 0 ? -d : d);
    }
    static Distance distance(const Node& a, const Node& b) {
        return add(square(difference(a.x, b.x)), square(difference(a.y, b.y)));
    }
    static bool by_y(const Node& a, const Node& b) {
        return a.y != b.y ? a.y < b.y : a.x < b.x;
    }
    void update(const Node& a, const Node& b) {
        const auto d = distance(a, b);
        if (d < best_) {
            best_ = d;
            best_first_ = std::min(a.index, b.index);
            best_second_ = std::max(a.index, b.index);
            zero_ = a.x == b.x && a.y == b.y;
        }
    }
    void solve(std::size_t left, std::size_t right) {
        if (zero_) return;
        if (right - left <= 3) {
            for (auto i = left; i < right; ++i)
                for (auto j = i + 1; j < right; ++j) update(nodes_[i], nodes_[j]);
            std::sort(nodes_.begin() + left, nodes_.begin() + right, by_y);
            return;
        }
        const auto middle = left + (right - left) / 2;
        const auto split_x = nodes_[middle].x;
        solve(left, middle);
        solve(middle, right);
        if (zero_) return;
        std::merge(nodes_.begin() + left, nodes_.begin() + middle,
                   nodes_.begin() + middle, nodes_.begin() + right,
                   buffer_.begin() + left, by_y);
        std::copy(buffer_.begin() + left, buffer_.begin() + right, nodes_.begin() + left);
        auto used = left;
        for (auto i = left; i < right; ++i) {
            const Node current = nodes_[i];
            if (!(square(difference(current.x, split_x)) < best_)) continue;
            for (auto j = used; j > left;) {
                const Node& previous = buffer_[--j];
                if (!(square(difference(current.y, previous.y)) < best_)) break;
                update(current, previous);
                if (zero_) return;
            }
            buffer_[used++] = current;
        }
    }

public:
    template<std::input_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
    static std::optional<NearestTwoPointsResult> run(Iterator first, Sentinel last) {
        NearestTwoPointsSolver solver;
        if constexpr (std::sized_sentinel_for<Sentinel, Iterator>) {
            const auto count = last - first;
#ifndef SKIP_RUNTIME_CHECKS
            if (count < 0) throw std::invalid_argument("nearest_two_points: reversed range");
#endif
            if (std::cmp_greater(count, solver.nodes_.max_size()))
                throw std::length_error("nearest_two_points: too many points");
            solver.nodes_.reserve(static_cast<std::size_t>(count));
        }
        for (; first != last; ++first) {
            const auto& point = *first;
            static_assert(std::integral<std::remove_cvref_t<decltype(point.first)>> &&
                          std::integral<std::remove_cvref_t<decltype(point.second)>>,
                          "nearest_two_points requires integral pair coordinates");
#ifndef SKIP_RUNTIME_CHECKS
            if (!valid_coordinate(point.first) || !valid_coordinate(point.second))
                throw std::out_of_range("nearest_two_points: coordinates must fit int32_t");
#endif
            if (solver.nodes_.size() == solver.nodes_.max_size())
                throw std::length_error("nearest_two_points: too many points");
            solver.nodes_.push_back({static_cast<std::int32_t>(point.first),
                                     static_cast<std::int32_t>(point.second),
                                     solver.nodes_.size()});
        }
        if (solver.nodes_.size() < 2) return std::nullopt;
        std::sort(solver.nodes_.begin(), solver.nodes_.end(), [](const Node& a, const Node& b) {
            return a.x != b.x ? a.x < b.x : a.y < b.y;
        });
        solver.best_ = distance(solver.nodes_[0], solver.nodes_[1]);
        solver.best_first_ = std::min(solver.nodes_[0].index, solver.nodes_[1].index);
        solver.best_second_ = std::max(solver.nodes_[0].index, solver.nodes_[1].index);
        solver.zero_ = solver.nodes_[0].x == solver.nodes_[1].x &&
                       solver.nodes_[0].y == solver.nodes_[1].y;
        if (!solver.zero_) {
            solver.buffer_.resize(solver.nodes_.size());
            solver.solve(0, solver.nodes_.size());
        }
        return NearestTwoPointsResult{narrow(solver.best_), solver.best_first_, solver.best_second_};
    }
};
} // namespace reinsirk::detail
#endif

#ifndef SIRKIT_NEAREST_TWO_POINTS_DEFINED
#define SIRKIT_NEAREST_TWO_POINTS_DEFINED
// [first,last)の整数pair列から最近点対を返す。int32座標・未整列可・入力不変、O(N log N)時間/O(N)領域。
// 2点未満はnullopt。同距離の対は任意。最小二乗距離がuint64_tを超える場合は常時overflow_error。
template<std::input_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
std::optional<NearestTwoPointsResult> nearest_two_points(Iterator first, Sentinel last) {
    return reinsirk::detail::NearestTwoPointsSolver::run(first, last);
}

// firstからcount個の点を処理する。同じ戻り値・計算量。count>=0で、指定個数を参照できること。
template<std::input_iterator Iterator>
std::optional<NearestTwoPointsResult> nearest_two_points(Iterator first, std::ptrdiff_t count) {
#ifndef SKIP_RUNTIME_CHECKS
    if (count < 0) throw std::invalid_argument("nearest_two_points: negative count");
#endif
    return nearest_two_points(std::counted_iterator(first, count), std::default_sentinel);
}

// vector全体を処理する。同じ戻り値・計算量。元入力の点番号を結果に保持する。
template<class Point, class Allocator>
std::optional<NearestTwoPointsResult> nearest_two_points(const std::vector<Point, Allocator>& points) {
    return nearest_two_points(points.begin(), points.end());
}
#endif

#ifndef SIRKIT_CLOSEST_PAIR_DEFINED
#define SIRKIT_CLOSEST_PAIR_DEFINED
// 互換呼出: firstからn個の最短距離二乗をuint64_tで返す。n<2はINT32_MAX（負数は検査時に例外）。
// int32座標・未整列可・入力不変、O(N log N)時間/O(N)領域。uint64_t超過は常時overflow_error。
template<std::input_iterator Iterator>
std::uint64_t closest_pair(Iterator first, int n) {
    const auto result = nearest_two_points(first, static_cast<std::ptrdiff_t>(n));
    return result ? result->distance_squared : std::numeric_limits<std::int32_t>::max();
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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

int main() {
    int tests; verify_in >> tests;
    while (tests--) {
        int n; verify_in >> n;
        std::vector<P> points(n);
        for (auto& [x, y] : points) verify_in >> x >> y;
        const auto answer = nearest_two_points(points);
        verify_out << answer->first_index << ' ' << answer->second_index << '\n';
    }
}
