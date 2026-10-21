// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

int main() {
    int n{};
    verify_in >> n;
    std::vector<std::pair<std::int64_t, std::int64_t>> points(n);
    for (auto& [x, y] : points) verify_in >> x >> y;
    std::sort(points.begin(), points.end(), arccmp_less);
    // ライブラリの[0,2π)順を、問題の(-π,π]順に合わせる。
    const auto split = std::find_if(points.begin(), points.end(), [](const auto& p) { return p.second < 0; });
    std::rotate(points.begin(), split, points.end());
    for (const auto& [x, y] : points) verify_out << x << ' ' << y << '\n';
}
