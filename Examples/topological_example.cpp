// SPDX-License-Identifier: CC0-1.0
#include "../Graph/Topological.cpp"
#include <iostream>
int main() {
    const Graph graph{{3}, {2}, {}, {}};
    for (int v : TopologicalSort(graph)) std::cout << v << ' '; // 0 1 3 2
    std::cout << '\n';
    const auto result = TopologicalSortDetails(graph);
    std::cout << result.acyclic << ' ' << result.unique << '\n'; // 1 0
    for (int position : result.position) std::cout << position << ' '; // 0 1 3 2
    std::cout << '\n' << IsDAG(Graph{{1}, {0}}) << '\n'; // 0
}
