// SPDX-License-Identifier: CC0-1.0
#include "../Geometry/偏角ソート.cpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

constexpr auto angle_limit = std::numeric_limits<std::int64_t>::max();
constexpr auto angle_minimum = std::numeric_limits<std::int64_t>::min();
static_assert(angle_cross_sign({angle_minimum, angle_limit}, {angle_limit, angle_minimum}) == 1);
static_assert(arccmp_less({angle_limit, angle_limit - 1}, {angle_limit, angle_limit}));

int main() {
    using Point = std::pair<std::int64_t, std::int64_t>;
    const std::vector<Point> points{{0, -1}, {2, 0}, {-1, 0}, {0, 0}, {0, 1}, {1, 0}};
    auto sorted = points;
    angle_sort(sorted); // 零、+x、+y、-x、-yの順。同方向の長さ順は任意。
    assert(sorted.front() == Point(0, 0));
    for (std::size_t i = 1; i < sorted.size(); ++i)
        assert(arccmp_less_eq(sorted[i - 1], sorted[i]));
    assert(same_angle(Point(1, 0), Point(2, 0)));
    assert(!arccmp_less(Point(1, 0), Point(2, 0)));
    const auto indices = angle_sort_indices(points, true);
    for (std::size_t i = 1; i < indices.size(); ++i)
        assert(arccmp_g_eq(points[indices[i - 1]], points[indices[i]]));
    std::sort(sorted.begin(), sorted.end(), arccmp_g); // 既存の比較関数でも利用可。
    assert(sorted.back() == Point(0, 0));
    const auto limit = std::numeric_limits<std::int64_t>::max();
    assert(angle_cross_sign(Point(limit, limit), Point(-limit, limit)) == 1);
}
