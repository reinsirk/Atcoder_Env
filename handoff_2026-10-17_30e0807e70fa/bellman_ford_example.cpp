// SPDX-License-Identifier: CC0-1.0
// Original source: https://zenn.dev/reputeless/books/standard-cpp-for-competitive-programming/viewer/bellman-ford
// Modified by reinsirk with assistance from OpenAI Codex.
#ifndef REINSIRK_BELLMAN_FORD_DEFINED
#define REINSIRK_BELLMAN_FORD_DEFINED

#include <algorithm>
#include <cstddef>
#include <cstdint>
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

// 始点から到達できる負閉路があればtrue。未到達はINF、負閉路の影響下は-INF。
// distancesは全上書き。頂点数はint、有限距離は(-INF, INF)に収まること。
// 重みは[-1e9, 1e9]。標準C++17、O(VE+V+E)時間・O(V)追加領域。
// 入力範囲の実行時チェックなし。
inline bool BellmanFord(const Graph& graph, std::vector<std::int64_t>& distances,
                        int start_index) {
  constexpr std::int64_t max_cost = 1'000'000'000;
  const int n = static_cast<int>(graph.size());
  std::vector<int> sources;
  for (int from = 0; from < n; ++from) {
    if (!graph[from].empty()) sources.push_back(from);
  }

  // 単純路は(n-1)辺以下。候補も[-(n+1), n+2]*max_cost内なので加算は安全。
  const auto count = static_cast<std::int64_t>(n);
  const std::int64_t unreachable = (count + 1) * max_cost;
  const std::int64_t lower_bound = -count * max_cost;
  std::vector<std::int64_t> distance(graph.size(), unreachable);
  std::vector<unsigned char> negative(graph.size(), 0);
  std::vector<int> queue;
  queue.reserve(graph.size());
  distance[start_index] = 0;

  const auto mark_negative = [&](int vertex) {
    if (negative[vertex]) return;
    negative[vertex] = 1;
    queue.push_back(vertex);
  };
  bool converged = false;
  for (int pass = 1; pass < n; ++pass) {
    bool changed = false;
    for (const int from : sources) {
      if (distance[from] == unreachable) continue;
      for (const auto& edge : graph[from]) {
        std::int64_t candidate = distance[from] + edge.w;
        if (candidate >= distance[edge.to]) continue;
        // 単純路では届かない下限。負閉路の種を記録してから丸める。
        if (candidate <= lower_bound) mark_negative(edge.to);
        candidate = std::max(candidate, lower_bound);
        if (candidate < distance[edge.to]) {
          distance[edge.to] = candidate;
          changed = true;
        }
      }
    }
    if (!changed) {
      converged = true;
      break;
    }
  }

  // さらに短くできる頂点を、負閉路の影響の種として記録する。
  if (!converged) {
    for (const int from : sources) {
      if (distance[from] == unreachable) continue;
      for (const auto& edge : graph[from])
        if (distance[from] + edge.w < distance[edge.to]) mark_negative(edge.to);
    }
  }
  // 距離を加算せず、到達関係だけで負閉路の影響を伝播する。
  for (std::size_t head = 0; head < queue.size(); ++head)
    for (const auto& edge : graph[queue[head]]) mark_negative(edge.to);

  for (int vertex = 0; vertex < n; ++vertex) {
    if (negative[vertex]) {
      distance[vertex] = -INF;
    } else if (distance[vertex] == unreachable) {
      distance[vertex] = INF;
    }
  }
  distances.swap(distance);
  return !queue.empty();
}
#endif

#include <iostream>
#include <vector>

int main() {
  Graph graph(5);
  graph[0].emplace_back(1, 2);
  graph[1].emplace_back(2, -3);
  graph[2].emplace_back(1, 1);  // 負閉路。
  graph[2].emplace_back(3, 4);
  std::vector<std::int64_t> distances;
  const bool has_negative_cycle = BellmanFord(graph, distances, 0);
  std::cout << has_negative_cycle << '\n';  // 1
  for (const auto distance : distances) {
    if (distance == INF) std::cout << "unreachable\n";
    else if (distance == -INF) std::cout << "negative cycle\n";
    else std::cout << distance << '\n';
  }
}
