// SPDX-License-Identifier: CC0-1.0

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>

// ライブラリをここへ貼り付けても使える。
#include "../Geometry/Nearest_Two_points.cpp"

int main() {
    const std::vector<P> points{{9, 7}, {-3, 2}, {4, 5}, {5, 5}};
    const auto answer = nearest_two_points(points);
    assert(answer && answer->distance_squared == 1);
    assert(answer->first_index == 2 && answer->second_index == 3);
    std::cout << answer->distance_squared << ' ' << answer->first_index
              << ' ' << answer->second_index << '\n';

    // 始終イテレータ版と個数版。入力の並びは変わらない。
    assert(nearest_two_points(points.begin(), points.end())->distance_squared == 1);
    assert(nearest_two_points(points.begin(), std::ptrdiff_t{4})->distance_squared == 1);
    assert(closest_pair(points.begin(), 4) == 1);
    assert(points.front() == P(9, 7));

    const std::vector<P> empty;
    assert(!nearest_two_points(empty));
    assert(closest_pair(empty.begin(), 0) == std::numeric_limits<std::int32_t>::max());

    // 遠い対があっても、最短距離二乗が収まれば結果を返す。
    const std::vector<P> extremes{{std::numeric_limits<std::int32_t>::min(),
                                   std::numeric_limits<std::int32_t>::min()},
                                  {std::numeric_limits<std::int32_t>::max(),
                                   std::numeric_limits<std::int32_t>::max()},
                                  {std::numeric_limits<std::int32_t>::max() - 1,
                                   std::numeric_limits<std::int32_t>::max()}};
    assert(nearest_two_points(extremes)->distance_squared == 1);
}
