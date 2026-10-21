// SPDX-License-Identifier: CC0-1.0
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <utility>

int main() {
    std::ios::sync_with_stdio(false); std::cin.tie(nullptr);
    int n; if (!(std::cin >> n)) return 0;
    std::int64_t answer = 0;
    LineContainer2D points;
    for (int i = 0; i < n; ++i) {
        std::int64_t x, y; std::cin >> x >> y;
        points.add(x, y);
        answer = std::max({answer, points.max_rounded_ll(y, -x), -points.min_rounded_ll(y, -x)});
    }
    std::cout << answer << '\n';
}
