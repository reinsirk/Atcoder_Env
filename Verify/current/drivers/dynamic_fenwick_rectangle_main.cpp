// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

int main() {
    int n, q;
    if (!(verify_in >> n >> q)) return 0;
    using Point = std::array<std::int64_t, 3>;
    using Query = std::array<std::int64_t, 5>;
    std::vector<Point> initial(n);
    std::vector<Query> queries(q);
    std::vector<int> xs, ys;
    for (auto& p : initial) {
        verify_in >> p[0] >> p[1] >> p[2];
        xs.push_back(static_cast<int>(p[0])); ys.push_back(static_cast<int>(p[1]));
    }
    for (auto& c : queries) {
        verify_in >> c[0] >> c[1] >> c[2] >> c[3];
        if (c[0]) verify_in >> c[4];
        else { xs.push_back(static_cast<int>(c[1])); ys.push_back(static_cast<int>(c[2])); }
    }
    std::sort(xs.begin(), xs.end()); xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
    std::sort(ys.begin(), ys.end()); ys.erase(std::unique(ys.begin(), ys.end()), ys.end());
    auto xi = [&](std::int64_t x) { return static_cast<int>(std::lower_bound(xs.begin(), xs.end(), x) - xs.begin()); };
    auto yi = [&](std::int64_t y) { return static_cast<int>(std::lower_bound(ys.begin(), ys.end(), y) - ys.begin()); };
    // 圧縮するのは初期点・追加点の座標だけ。照会の端点はlower_boundで変換する。
    // 原典と同じ外側Fenwickを組み、各行に今回の1次元BITを利用する。
    const int width = static_cast<int>(xs.size());
    std::vector<DynamicFenwickTree<int, std::int64_t>> rows;
    for (int i = 0; i <= width; ++i) rows.emplace_back(static_cast<int>(ys.size()));
    auto add = [&](int x, int y, std::int64_t w) {
        for (++x; x <= width; x += x & -x) rows[x].add(y, w);
    };
    auto rectangle = [&](int l, int d, int r, int u) {
        std::int64_t answer = 0;
        while (l != r) {
            if (l < r) { answer += rows[r].sum(d, u); r -= r & -r; }
            else { answer -= rows[l].sum(d, u); l -= l & -l; }
        }
        return answer;
    };
    for (const auto& p : initial) add(xi(p[0]), yi(p[1]), p[2]);
    for (const auto& c : queries) {
        if (c[0] == 0) add(xi(c[1]), yi(c[2]), c[3]);
        else verify_out << rectangle(xi(c[1]), yi(c[2]), xi(c[3]), yi(c[4])) << '\n';
    }
}
