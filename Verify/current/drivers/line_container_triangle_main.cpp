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
    // 原典の2次元照会を、1次元包絡と軸方向の最大・最小値から組み立てる。
    std::vector<std::pair<std::int64_t, std::int64_t>> points(n);
    MaxLineContainer<long double> high;
    MinLineContainer<long double> low;
    std::int64_t ymin = 1'000'001, ymax = -1'000'001;
    for (auto& [x, y] : points) {
        std::cin >> x >> y;
        high.add(y, x); low.add(y, x);
        ymin = std::min(ymin, y); ymax = std::max(ymax, y);
    }
    for (auto [x, y] : points) {
        long double a, b;
        if (y == 0) { a = -static_cast<long double>(x) * ymin; b = -static_cast<long double>(x) * ymax; }
        else {
            const auto ratio = -static_cast<long double>(x) / y;
            a = static_cast<long double>(y) * high.query(ratio);
            b = static_cast<long double>(y) * low.query(ratio);
        }
        answer = std::max({answer, static_cast<std::int64_t>(std::llround(std::fabs(a))),
                                  static_cast<std::int64_t>(std::llround(std::fabs(b)))});
    }
    std::cout << answer << '\n';
}
