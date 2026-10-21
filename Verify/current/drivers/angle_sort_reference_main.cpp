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
    using Point = std::pair<std::int64_t, std::int64_t>;
    std::vector<Point> points;
    for (int x = -8; x <= 8; ++x) for (int y = -8; y <= 8; ++y) points.push_back({x, y});
    auto argument = [](Point p) {
        if (p == Point{0, 0}) return -1.0L;
        long double result = std::atan2(static_cast<long double>(p.second), static_cast<long double>(p.first));
        return result < 0 ? result + 2 * std::acos(-1.0L) : result;
    };
    for (auto a : points) for (auto b : points) {
        const auto aa = argument(a), bb = argument(b);
        if (std::abs(aa - bb) > 1e-15L) assert(arccmp_less(a, b) == (aa < bb));
        assert(arccmp_g(a, b) == arccmp_less(b, a));
        assert(arccmp_less_eq(a, b) == !arccmp_less(b, a));
        assert(arccmp_g_eq(a, b) == !arccmp_less(a, b));
    }
    auto ascending = points; angle_sort(ascending);
    auto descending = points; angle_sort(descending, true);
    const auto indices = angle_sort_indices(points);
    const auto reverse_indices = angle_sort_indices(points, true);
    for (std::size_t i = 1; i < points.size(); ++i) {
        assert(!arccmp_less(ascending[i], ascending[i - 1]));
        assert(!arccmp_g(descending[i], descending[i - 1]));
        assert(!arccmp_less(points[indices[i]], points[indices[i - 1]]));
        assert(!arccmp_g(points[reverse_indices[i]], points[reverse_indices[i - 1]]));
    }
    const auto high = std::numeric_limits<std::int64_t>::max();
    assert(angle_cross_sign({high, high - 1}, {high - 1, high}) == 1);
    assert(angle_cross_sign({INT64_MIN, 0}, {0, INT64_MIN}) == 1);
    assert(same_angle({1, 1}, {high, high}));
    std::cout << "OK\n";
}
