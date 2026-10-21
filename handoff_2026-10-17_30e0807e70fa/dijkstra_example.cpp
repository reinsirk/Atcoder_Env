// SPDX-License-Identifier: CC0-1.0
// Original source: https://zenn.dev/reputeless/books/standard-cpp-for-competitive-programming/viewer/dijkstra
// Modified by reinsirk with assistance from OpenAI Codex.
#ifndef REINSIRK_DIJKSTRA_DEFINED
#define REINSIRK_DIJKSTRA_DEFINED
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <queue>
#include <utility>
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

// 共通処理。距離のみの場合、経路用の確保と更新をコンパイル時に除く。
template <bool RestorePath>
inline void DijkstraImpl(const Graph& graph, std::vector<std::int64_t>& distances,
                         int start_index, int target_index, std::vector<int>* path) {
  constexpr std::int64_t max_weight = 1'000'000'000;
  const int n = static_cast<int>(graph.size());

  // 単純路は(n-1)辺以下。未到達値への重み加算もint64_t内に収まる。
  const std::int64_t unreachable = (static_cast<std::int64_t>(n) + 1) * max_weight;
  std::vector<std::int64_t> distance(graph.size(), unreachable);
  std::vector<int> previous, result;
  if constexpr (RestorePath) previous.assign(graph.size(), -1);
  using State = std::pair<std::int64_t, int>;
  std::priority_queue<State, std::vector<State>, std::greater<State>> queue;
  distance[start_index] = 0;
  queue.emplace(0, start_index);

  while (!queue.empty()) {
    const auto [current, from] = queue.top();
    queue.pop();
    if (current != distance[from]) continue;
    for (const auto& edge : graph[from]) {
      const std::int64_t candidate = current + edge.w;
      if (candidate >= distance[edge.to]) continue;
      distance[edge.to] = candidate;
      if constexpr (RestorePath) previous[edge.to] = from;
      queue.emplace(candidate, edge.to);
    }
  }

  if constexpr (RestorePath) {
    if (distance[target_index] != unreachable) {
      for (int vertex = target_index; vertex != -1; vertex = previous[vertex])
        result.push_back(vertex);
      std::reverse(result.begin(), result.end());
    }
  }
  for (auto& value : distance) {
    if (value == unreachable) value = INF;
  }
  distances.swap(distance);
  if constexpr (RestorePath) path->swap(result);
}

// 全頂点の最短距離を上書きする。重みは[0, 1e9]、未到達はINF。
// O(V+E log(E+1))時間・O(V+E)追加領域。入力範囲の実行時チェックなし。
// 頂点数はint、有限距離は[0, INF)に収まること。
inline void Dijkstra(const Graph& graph, std::vector<std::int64_t>& distances,
                     int start_index) {
  DijkstraImpl<false>(graph, distances, start_index, -1, nullptr);
}

// 全頂点の距離とstart_indexからtarget_indexへの頂点列を上書きする。
// 到達不能ならpathは空。距離の仕様と計算量は3引数版と同じ。
inline void Dijkstra(const Graph& graph, std::vector<std::int64_t>& distances,
                     int start_index, int target_index, std::vector<int>& path) {
  DijkstraImpl<true>(graph, distances, start_index, target_index, &path);
}
#endif

#include <iostream>

int main() {
  Graph graph(5);
  graph[0].emplace_back(1, 2);
  graph[0].emplace_back(2, 7);
  graph[1].emplace_back(2, 3);
  graph[2].emplace_back(3, 0);

  std::vector<std::int64_t> distances;
  std::vector<int> path;
  Dijkstra(graph, distances, 0, 3, path);

  for (const auto distance : distances) {
    if (distance == INF) std::cout << "INF\n";
    else std::cout << distance << '\n';
  }
  for (std::size_t i = 0; i < path.size(); ++i)
    std::cout << (i == 0 ? "" : " ") << path[i];
  std::cout << '\n';
}

