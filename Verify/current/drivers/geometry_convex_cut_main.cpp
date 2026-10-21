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
    int n; std::cin >> n;
    std::vector<geo::PointLD> polygon(n);
    for (auto& p : polygon) std::cin >> p.x >> p.y;
    std::cout << std::fixed << std::setprecision(12);
    int q; std::cin >> q;
    while (q--) {
        geo::Line<long double> line;
        std::cin >> line.a.x >> line.a.y >> line.b.x >> line.b.y;
        std::cout << geo::area(geo::convex_cut(polygon, line)) << '\n';
    }
}
