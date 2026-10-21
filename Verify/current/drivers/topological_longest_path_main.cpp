// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <algorithm>
#include <vector>

int main() {
    int n, m; verify_in >> n >> m;
    Graph graph(n);
    while (m--) {
        int x, y; verify_in >> x >> y;
        graph[x - 1].push_back(y - 1);
    }
    std::vector<int> length(n);
    for (int from : TopologicalSort(graph))
        for (int to : graph[from])
            length[to] = std::max(length[to], length[from] + 1);
    verify_out << *std::max_element(length.begin(), length.end()) << '\n';
}
