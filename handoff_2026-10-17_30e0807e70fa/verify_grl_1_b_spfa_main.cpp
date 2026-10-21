#include <cstdint>
#include <iostream>
#include <vector>

// ここにSPFAのライブラリを貼り付ける。

int main() {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  int vertices, edges, start;
  if (!(std::cin >> vertices >> edges >> start)) return 0;
  Graph graph(vertices);
  for (int i = 0; i < edges; ++i) {
    int from, to;
    std::int64_t w;
    std::cin >> from >> to >> w;
    graph[from].emplace_back(to, w);
  }

  std::vector<std::int64_t> distances;
  if (SPFA(graph, distances, start)) {
    std::cout << "NEGATIVE CYCLE\n";
    return 0;
  }
  for (const std::int64_t distance : distances) {
    if (distance == INF) std::cout << "INF\n";
    else std::cout << distance << '\n';
  }
  return 0;
}
