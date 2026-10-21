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
    geo::Line<long double> line; std::cin >> line.a.x >> line.a.y >> line.b.x >> line.b.y;
    int q; std::cin >> q;
    std::cout << std::fixed << std::setprecision(12);
    while (q--) {
        geo::PointLD p; std::cin >> p.x >> p.y;
        const auto answer = geo::reflection(p, line);
        std::cout << answer.x << ' ' << answer.y << '\n';
    }
}
