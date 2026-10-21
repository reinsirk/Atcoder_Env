// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

int main() {
    geo::Circle c; std::cin >> c.center.x >> c.center.y >> c.radius;
    std::cout << std::fixed << std::setprecision(12);
    geo::Circle d; std::cin >> d.center.x >> d.center.y >> d.radius;
    auto result = geo::intersection(c, d);
    auto& points = result.points;
    if (points.empty()) return 1;
    if (points.size() == 1) points.push_back(points[0]);
    std::sort(points.begin(), points.end());
    std::cout << points[0].x << ' ' << points[0].y << ' ' << points[1].x << ' ' << points[1].y << '\n';
}
