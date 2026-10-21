// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>

int main() {
    int n{}, m{}; verify_in >> n >> m;
    std::vector distance(n, std::vector<std::int64_t>(n, INF));
    for (int v = 0; v < n; ++v) distance[v][v] = 0;
    while (m--) {
        int a{}, b{}; std::int64_t time{}; verify_in >> a >> b >> time;
        distance[a - 1][b - 1] = distance[b - 1][a - 1] = time;
    }
    FloydWarshall(distance);
    std::int64_t answer = INF;
    for (const auto& row : distance)
        answer = std::min(answer, *std::max_element(row.begin(), row.end()));
    verify_out << answer << '\n';
}
