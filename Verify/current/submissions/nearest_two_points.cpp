#define SKIP_RUNTIME_CHECKS
// Verify: https://judge.yosupo.jp/problem/closest_pair
// Submission: https://judge.yosupo.jp/submission/413074

// Library: Geometry/Nearest_Two_points.cpp
// SPDX-License-Identifier: CC0-1.0
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/nearest_two_points.md
//
// API一覧
// [最近点対]
// P: pair<int32_t,int32_t>の別名。NearestTwoPointsResultはdistance_squared:uint64_tとfirst_index/second_index:size_tを所有。
// NearestTwoPointsResult::distance_squaredは最短距離の二乗、添字は元入力の0始まり（first_index < second_index）。
// nearest_two_points(first,last): [first,last)の整数pair-like列からoptional<NearestTwoPointsResult>を返す。int32座標・未整列可・入力不変、O(N log N)時間/O(N)領域。
// 2点未満はnullopt。同距離の対は任意。最小二乗距離がuint64_tを超える場合は常時overflow_error。
// nearest_two_points(first,count): firstからptrdiff_t型のcount個を処理する。同じ戻り値・計算量。count>=0で、指定個数を参照できること。
// nearest_two_points(points): vector全体を処理する。同じ戻り値・計算量。元入力の点番号を結果に保持する。
// closest_pair(first,n): firstからn個の最短距離二乗をuint64_tで返す。n<2はINT32_MAX（負数は検査時に例外）。
// int32座標・未整列可・入力不変、O(N log N)時間/O(N)領域。uint64_t超過は常時overflow_error。
// nearest_two_points_sorted(first,last): 整列済み範囲版。(x,y)辞書順の非減少を前提に初回sortを省く。検査時はコピー中に順序を確認。
// 戻り値はnearest_two_pointsと同じで、添字は渡した列内。O(N log N)時間/O(N)領域、入力不変。
// nearest_two_points_sorted(first,count): 整列済みfirstからcount個の版。count>=0かつ指定個数を参照可能。戻り値・計算量は範囲版と同じ。
// nearest_two_points_sorted(points): 整列済みvector全体版。戻り値・前提・計算量は範囲版と同じ。

#include <algorithm>
#include <array>
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

struct NearestTwoPointsResult {
    std::uint64_t distance_squared;
    std::size_t first_index;
    std::size_t second_index;
};
#endif

#ifndef SIRKIT_INTERNAL_NEAREST_TWO_POINTS_DEFINED
#define SIRKIT_INTERNAL_NEAREST_TWO_POINTS_DEFINED
namespace reinsirk::detail {
template<class Index>
class NearestTwoPointsSolver {
    struct Node {
        std::int32_t x;
        std::int32_t y;
        Index index;
        // 作業領域は各整列パスで全要素を代入してから読む。
        Node() noexcept {}
        Node(std::int32_t a, std::int32_t b, Index i) noexcept : x(a), y(b), index(i) {}
    };
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && \
    !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
    // GCC/Clangの整数拡張。標準C++の型ではない。
    __extension__ using Distance = unsigned __int128;
    // 差はUINT32_MAX以下なので、単独の二乗は64bitに収まる。
    static Distance square(std::uint64_t d) { return Distance(d * d); }
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
#if defined(__GNUC__) && !defined(__clang__)
            // GCCでは計算済みの65bit距離を使う。Clangは座標比較を維持する。
            zero_ = !(square(0) < d);
#else
            zero_ = a.x == b.x && a.y == b.y;
#endif
        }
    }
    // 符号bitを反転したxを10/11/11bitの3パスで基数整列する。
    void sort_by_x() {
        // Indexの範囲内の点数を扱うので、合計と最終offsetも同じ型に収まる。
        std::array<std::array<Index, 2048>, 3> counts{};
        for (const auto& node : nodes_) {
            const auto key = static_cast<std::uint32_t>(node.x) ^ UINT32_C(0x80000000);
            ++counts[0][key & 1023U];
            ++counts[1][(key >> 10) & 2047U];
            ++counts[2][key >> 21];
        }
        buffer_.resize(nodes_.size());
        Node* input = nodes_.data();
        Node* output = buffer_.data();
        for (unsigned pass = 0; pass < 3; ++pass) {
            auto& offsets = counts[pass];
            std::size_t active = 0;
            for (const auto count : offsets) active += count != 0;
            if (active <= 1) continue;
            Index sum = 0;
            for (auto& count : offsets) { const auto n = count; count = sum; sum += n; }
            for (std::size_t i = 0; i < nodes_.size(); ++i) {
                const auto key = static_cast<std::uint32_t>(input[i].x) ^ UINT32_C(0x80000000);
                output[offsets[(key >> (pass == 0 ? 0 : pass == 1 ? 10 : 21)) & (pass == 0 ? 1023U : 2047U)]++] = input[i];
            }
            std::swap(input, output);
        }
        if (input != nodes_.data()) nodes_.swap(buffer_);
    }
    // 同区間の両配列をx順で開始し、outputをy順で返す。inputは作業用。
    void solve(Node* input, Node* output, std::size_t left, std::size_t right) {
        if (zero_) return;
        if (right - left <= 16) {
            for (auto i = left; i < right; ++i)
                for (auto j = i + 1; j < right; ++j) update(output[i], output[j]);
            std::sort(output + left, output + right, by_y);
            return;
        }
        const auto middle = left + (right - left) / 2;
        const auto split_x = input[middle].x;
        solve(output, input, left, middle);
        solve(output, input, middle, right);
        if (zero_) return;
        // 左半分のxは右半分以下。同じyでは左を先にすればよい。
        std::merge(input + left, input + middle, input + middle, input + right,
                   output + left, [](const Node& a, const Node& b) { return a.y < b.y; });
        auto used = left;
        for (auto i = left; i < right; ++i) {
            const Node current = output[i];
            if (!(square(difference(current.x, split_x)) < best_)) continue;
            for (auto j = used; j > left;) {
                const Node& previous = input[--j];
                if (!(square(difference(current.y, previous.y)) < best_)) break;
                update(current, previous);
                if (zero_) return;
            }
            input[used++] = current;
        }
    }

public:
    template<bool Sorted = false, std::input_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
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
#ifndef SKIP_RUNTIME_CHECKS
            if constexpr (Sorted) {
                if (!solver.nodes_.empty()) {
                    const auto& previous = solver.nodes_.back();
                    if (static_cast<std::int32_t>(point.first) < previous.x ||
                        (static_cast<std::int32_t>(point.first) == previous.x &&
                         static_cast<std::int32_t>(point.second) < previous.y))
                        throw std::invalid_argument("nearest_two_points_sorted: unordered input");
                }
            }
#endif
            solver.nodes_.push_back({static_cast<std::int32_t>(point.first),
                                     static_cast<std::int32_t>(point.second),
                                     static_cast<Index>(solver.nodes_.size())});
        }
        if (solver.nodes_.size() < 2) return std::nullopt;
        if constexpr (!Sorted) {

            if (solver.nodes_.size() >= 2048) {
                if (solver.nodes_[0].x == solver.nodes_[1].x &&
                    solver.nodes_[0].y == solver.nodes_[1].y)
                    return NearestTwoPointsResult{0, 0, 1};
                solver.sort_by_x();
            }
            else std::sort(solver.nodes_.begin(), solver.nodes_.end(), [](const Node& a, const Node& b) {
                return a.x != b.x ? a.x < b.x : a.y < b.y;
            });
        }
        solver.best_ = distance(solver.nodes_[0], solver.nodes_[1]);
        solver.best_first_ = std::min(solver.nodes_[0].index, solver.nodes_[1].index);
        solver.best_second_ = std::max(solver.nodes_[0].index, solver.nodes_[1].index);
        solver.zero_ = solver.nodes_[0].x == solver.nodes_[1].x &&
                       solver.nodes_[0].y == solver.nodes_[1].y;
        if (!solver.zero_) {
            // 最初だけ両配列のx順を揃える。
            solver.buffer_ = solver.nodes_;
            solver.solve(solver.nodes_.data(), solver.buffer_.data(), 0, solver.nodes_.size());
        }
        return NearestTwoPointsResult{narrow(solver.best_), solver.best_first_, solver.best_second_};
    }
};
} // namespace reinsirk::detail
#endif

#ifndef SIRKIT_NEAREST_TWO_POINTS_DEFINED
#define SIRKIT_NEAREST_TWO_POINTS_DEFINED
template<std::input_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
std::optional<NearestTwoPointsResult> nearest_two_points(Iterator first, Sentinel last) {
    // 小入力はsize_t経路を保ち、大入力だけ点ごとの添字を圧縮する。
    if constexpr (std::sized_sentinel_for<Sentinel, Iterator>) {
        const auto n = last - first;
        if (n >= 2048 && std::cmp_less_equal(n, std::numeric_limits<std::uint32_t>::max()))
            return reinsirk::detail::NearestTwoPointsSolver<std::uint32_t>::run(std::move(first), std::move(last));
    }
    return reinsirk::detail::NearestTwoPointsSolver<std::size_t>::run(std::move(first), std::move(last));
}

template<std::input_iterator Iterator>
std::optional<NearestTwoPointsResult> nearest_two_points(Iterator first, std::ptrdiff_t count) {
#ifndef SKIP_RUNTIME_CHECKS
    if (count < 0) throw std::invalid_argument("nearest_two_points: negative count");
#endif
    return nearest_two_points(std::counted_iterator(std::move(first), count), std::default_sentinel);
}

template<class Point, class Allocator>
std::optional<NearestTwoPointsResult> nearest_two_points(const std::vector<Point, Allocator>& points) {
    return nearest_two_points(points.begin(), points.end());
}
#endif

#ifndef SIRKIT_CLOSEST_PAIR_DEFINED
#define SIRKIT_CLOSEST_PAIR_DEFINED
template<std::input_iterator Iterator>
std::uint64_t closest_pair(Iterator first, int n) {
    const auto result = nearest_two_points(std::move(first), static_cast<std::ptrdiff_t>(n));
    return result ? result->distance_squared : std::numeric_limits<std::int32_t>::max();
}
#endif

#ifndef SIRKIT_NEAREST_TWO_POINTS_SORTED_DEFINED
#define SIRKIT_NEAREST_TWO_POINTS_SORTED_DEFINED
template<std::input_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
std::optional<NearestTwoPointsResult> nearest_two_points_sorted(Iterator first, Sentinel last) {
    // 小入力はsize_t経路を保ち、大入力だけ点ごとの添字を圧縮する。
    if constexpr (std::sized_sentinel_for<Sentinel, Iterator>) {
        const auto n = last - first;
        if (n >= 2048 && std::cmp_less_equal(n, std::numeric_limits<std::uint32_t>::max()))
            return reinsirk::detail::NearestTwoPointsSolver<std::uint32_t>::run<true>(std::move(first), std::move(last));
    }
    return reinsirk::detail::NearestTwoPointsSolver<std::size_t>::run<true>(std::move(first), std::move(last));
}
template<std::input_iterator Iterator>
std::optional<NearestTwoPointsResult> nearest_two_points_sorted(Iterator first, std::ptrdiff_t count) {
#ifndef SKIP_RUNTIME_CHECKS
    if (count < 0) throw std::invalid_argument("nearest_two_points_sorted: negative count");
#endif
    return nearest_two_points_sorted(std::counted_iterator(std::move(first), count), std::default_sentinel);
}
template<class Point, class Allocator>
std::optional<NearestTwoPointsResult> nearest_two_points_sorted(const std::vector<Point, Allocator>& points) {
    return nearest_two_points_sorted(points.begin(), points.end());
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
