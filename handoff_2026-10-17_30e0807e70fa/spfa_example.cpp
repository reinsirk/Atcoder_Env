// SPDX-License-Identifier: CC0-1.0
// Original source: https://zenn.dev/reputeless/books/standard-cpp-for-competitive-programming/viewer/bellman-ford
// Modified by reinsirk with assistance from OpenAI Codex.
#ifndef REINSIRK_SPFA_DEFINED
#define REINSIRK_SPFA_DEFINED
#include <cstddef>
#include <cstdint>
#include <queue>
#include <vector>

#ifndef REINSIRK_GRAPH_TYPES_DEFINED
#define REINSIRK_GRAPH_TYPES_DEFINED
inline constexpr std::int64_t INF = std::int64_t{1} << 60;

// 有向辺。出発点は隣接リストの添字。
struct Edge {
  // 行先
  int to;
  // コスト
  std::int64_t w;
  Edge(int destination, std::int64_t weight) : to(destination), w(weight) {}
};
using Graph = std::vector<std::vector<Edge>>;
#endif

// 更新された頂点だけをFIFOキューで処理するSPFA。標準C++17、重みは[-1e9, 1e9]。
// 到達可能な負閉路があればtrue。距離は成功時に全上書きし、未到達INF・負影響-INF。
// 最悪O(VE+V+E)時間・O(V)追加領域。入力範囲の実行時チェックなし。
// 頂点数はint、有限距離は(-INF, INF)に収まること。
inline bool SPFA(const Graph& graph, std::vector<std::int64_t>& distances,
                 int start_index) {
  constexpr std::int64_t max_cost = 1'000'000'000;
  const int n = static_cast<int>(graph.size());

  const std::int64_t unreachable = (static_cast<std::int64_t>(n) + 1) * max_cost;
  std::vector<std::int64_t> distance(graph.size(), unreachable);
  std::vector<int> path_edges(graph.size(), 0), affected;
  std::vector<std::uint8_t> in_queue(graph.size(), 0), negative(graph.size(), 0);
  affected.reserve(graph.size());
  std::queue<int> queue;
  distance[start_index] = 0;
  queue.push(start_index);
  in_queue[start_index] = 1;

  // 負閉路の下流を一度だけ走査し、以後は距離の緩和から除外する。
  const auto mark_negative = [&](int seed) {
    if (negative[seed]) return;
    const std::size_t first = affected.size();
    negative[seed] = 1;
    affected.push_back(seed);
    for (std::size_t head = first; head < affected.size(); ++head) {
      for (const auto& edge : graph[affected[head]]) {
        if (negative[edge.to]) continue;
        negative[edge.to] = 1;
        affected.push_back(edge.to);
      }
    }
  };

  while (!queue.empty()) {
    const int from = queue.front();
    queue.pop();
    in_queue[from] = 0;
    if (negative[from]) continue;
    for (const auto& edge : graph[from]) {
      if (negative[edge.to]) continue;
      // 格納する経路はn辺未満なので、候補の加算もint64_t内に収まる。
      const std::int64_t candidate = distance[from] + edge.w;
      if (candidate >= distance[edge.to]) continue;
      const int length = path_edges[from] + 1;
      if (length >= n) {
        mark_negative(edge.to);
        if (negative[from]) break;
        continue;
      }
      distance[edge.to] = candidate;
      path_edges[edge.to] = length;
      if (!in_queue[edge.to]) {
        queue.push(edge.to);
        in_queue[edge.to] = 1;
      }
    }
  }

  for (int vertex = 0; vertex < n; ++vertex) {
    if (negative[vertex]) distance[vertex] = -INF;
    else if (distance[vertex] == unreachable) distance[vertex] = INF;
  }
  distances.swap(distance);
  return !affected.empty();
}
#endif

#include <iostream>
#include <vector>

int main() {
  Graph graph(5);
  graph[0].emplace_back(1, 2);
  graph[1].emplace_back(2, -3);
  graph[2].emplace_back(1, 1);
  graph[2].emplace_back(3, 4);
  std::vector<std::int64_t> distances;
  const bool has_negative_cycle = SPFA(graph, distances, 0);
  std::cout << has_negative_cycle << '\n';  // 1
  for (const auto distance : distances) {
    if (distance == INF) std::cout << "unreachable\n";
    else if (distance == -INF) std::cout << "negative cycle\n";
    else std::cout << distance << '\n';
  }
}
