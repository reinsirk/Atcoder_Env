#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

// ここにDijkstraのライブラリを貼り付ける。

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int n, m, s, t;
    std::cin >> n >> m >> s >> t;
    Graph graph(n);
    for (int i = 0; i < m; ++i) {
        int from, to;
        std::int64_t w;
        std::cin >> from >> to >> w;
        graph[from].emplace_back(to, w);
    }

    std::vector<std::int64_t> distances;
    std::vector<int> path;
    Dijkstra(graph, distances, s, t, path);

    if (distances[t] == INF) {
        std::cout << -1 << '\n';
        return 0;
    }
    std::cout << distances[t] << ' ' << path.size() - 1 << '\n';
    for (std::size_t i = 1; i < path.size(); ++i)
        std::cout << path[i - 1] << ' ' << path[i] << '\n';
}
