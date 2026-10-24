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
class NearestTwoPointsSolver {
    struct Node {
        std::int32_t x;
        std::int32_t y;
        std::size_t index;
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
            zero_ = a.x == b.x && a.y == b.y;
        }
    }
    // 大入力では符号bitを反転したxを基数整列する。
    void sort_by_x() {
        std::array<std::array<std::size_t, 256>, 4> counts{};
        for (const auto& node : nodes_) {
            const auto key = static_cast<std::uint32_t>(node.x) ^ UINT32_C(0x80000000);
            for (unsigned pass = 0; pass < 4; ++pass) ++counts[pass][(key >> (pass * 8)) & 255U];
        }
        buffer_.resize(nodes_.size());
        Node* input = nodes_.data();
        Node* output = buffer_.data();
        for (unsigned pass = 0; pass < 4; ++pass) {
            auto& offsets = counts[pass];
            std::size_t active = 0;
            for (const auto count : offsets) active += count != 0;
            if (active <= 1) continue;
            std::size_t sum = 0;
            for (auto& count : offsets) { const auto n = count; count = sum; sum += n; }
            for (std::size_t i = 0; i < nodes_.size(); ++i) {
                const auto key = static_cast<std::uint32_t>(input[i].x) ^ UINT32_C(0x80000000);
                output[offsets[(key >> (pass * 8)) & 255U]++] = input[i];
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
        std::merge(input + left, input + middle, input + middle, input + right, output + left, by_y);
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
                                     solver.nodes_.size()});
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
    return reinsirk::detail::NearestTwoPointsSolver::run(std::move(first), std::move(last));
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
    return reinsirk::detail::NearestTwoPointsSolver::run<true>(std::move(first), std::move(last));
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
