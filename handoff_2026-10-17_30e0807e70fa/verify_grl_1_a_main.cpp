#include <cstdint>
#include <iostream>
#include <vector>

// ここにDijkstraのライブラリを貼り付ける。

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int n, m, r;
    std::cin >> n >> m >> r;
    Graph graph(n);
    for (int i = 0; i < m; ++i) {
        int from, to;
        std::int64_t w;
        std::cin >> from >> to >> w;
        graph[from].emplace_back(to, w);
    }

    std::vector<std::int64_t> distances;
    Dijkstra(graph, distances, r);

    for (const auto distance : distances) {
        if (distance == INF) std::cout << "INF\n";
        else std::cout << distance << '\n';
    }
}
