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
