// Verify: ローカル独立検査

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
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <limits>
#include <random>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::mt19937 generator(1271234);
    for (int n = 0; n <= 100; ++n) {
        std::vector<P> points(n);
        for (auto& [x, y] : points) { x = static_cast<int>(generator() % 201) - 100; y = static_cast<int>(generator() % 201) - 100; }
        auto actual = nearest_two_points(points);
        assert(bool(actual) == (n >= 2));
        if (n < 2) { assert(closest_pair(points.begin(), n) == std::numeric_limits<std::int32_t>::max()); continue; }
        std::uint64_t expected = std::numeric_limits<std::uint64_t>::max();
        for (int i = 0; i < n; ++i) for (int j = 0; j < i; ++j) {
            std::int64_t dx = std::int64_t(points[i].first) - points[j].first, dy = std::int64_t(points[i].second) - points[j].second;
            expected = std::min(expected, static_cast<std::uint64_t>(dx * dx + dy * dy));
        }
        assert(actual->distance_squared == expected);
        const auto a = points[actual->first_index], b = points[actual->second_index];
        std::int64_t dx = std::int64_t(a.first) - b.first, dy = std::int64_t(a.second) - b.second;
        assert(static_cast<std::uint64_t>(dx * dx + dy * dy) == expected);
        assert(nearest_two_points(points.begin(), points.end())->distance_squared == expected);
        assert(nearest_two_points(points.begin(), n)->distance_squared == expected);
        assert(closest_pair(points.begin(), n) == expected);
    }
    const std::vector<P> extreme{{INT32_MIN, INT32_MIN}, {INT32_MAX, INT32_MAX}};
    bool overflow = false; try { (void)nearest_two_points(extreme); } catch (const std::overflow_error&) { overflow = true; }
    assert(overflow);
    std::cout << "OK\n";
}
