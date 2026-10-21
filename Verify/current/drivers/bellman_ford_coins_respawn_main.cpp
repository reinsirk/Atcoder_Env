// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>

int main() {
    int n{}, m{}; std::int64_t price{}; verify_in >> n >> m >> price;
    WGraph graph(n);
    while (m--) {
        int a{}, b{}; std::int64_t coins{}; verify_in >> a >> b >> coins;
        graph[a - 1].emplace_back(b - 1, price - coins);
    }
    std::vector<std::int64_t> distance;
    BellmanFord(graph, distance, 0);
    if (distance[n - 1] == -INF) verify_out << -1 << '\n';
    else verify_out << std::max(std::int64_t{0}, -distance[n - 1]) << '\n';
}
