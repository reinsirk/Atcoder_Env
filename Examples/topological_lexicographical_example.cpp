#include "../Graph/Topological_sort.cpp"
#include <iostream>
int main() {
    const Graph graph{{3}, {2}, {}, {}};
    for (int v : TopologicalSortLexicographical(graph)) std::cout << v << ' '; // 0 1 2 3
    std::cout << '\n';
    auto result = TopologicalSortDetails(graph, true);
    std::cout << result.acyclic << ' ' << result.unique << '\n'; // 1 0
    auto cyclic = TopologicalSortDetails(Graph{{1}, {0}, {}}, true);
    std::cout << cyclic.acyclic << ' ' << cyclic.order.size() << '\n'; // 0 1
}
