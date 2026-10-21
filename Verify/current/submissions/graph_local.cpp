// Verify: ローカル独立検査

// Library: Graph/Bellman-Ford.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://zenn.dev/reputeless/books/standard-cpp-for-competitive-programming/viewer/bellman-ford

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <vector>

#ifndef REINSIRK_INF_DEFINED
#define REINSIRK_INF_DEFINED
inline constexpr std::int64_t INF = std::int64_t{1} << 60;
#endif

#ifndef REINSIRK_GRAPH_TYPES_DEFINED
#define REINSIRK_GRAPH_TYPES_DEFINED
// 有向辺。出発点は隣接リストの添字。
struct Edge
{
    // 行先
    int to;
    // コスト
    std::int64_t w;
    Edge(int destination, std::int64_t weight) : to(destination), w(weight) {}
};
using WGraph = std::vector<std::vector<Edge>>;
#endif

// BellmanFord(graph, distances, start_index): 始点からの最短距離をdistancesに格納し、到達可能な負閉路があればtrueを返す。
// SKIP_RUNTIME_CHECKSを定義すると入力検査を省く。
// 検査は始点と処理中に参照する辺のみ。未到達頂点の出辺は走査しない。
// 始点から到達できる負閉路があればtrue。未到達はINF、負閉路の影響下は-INF。
// distancesは成功時に全上書き。有限距離が(-INF, INF)の範囲外ならoverflow_error。
// 重みは[-1e9, 1e9]。O(VE+V+E)時間・O(V)追加領域。
// 小規模・少数始点は隣接リストを走査し、多数パスでは更新頂点に処理を絞る。
#ifndef REINSIRK_BELLMAN_FORD_DEFINED
#define REINSIRK_BELLMAN_FORD_DEFINED
namespace reinsirk::detail {
// 3パス目以降は距離が更新された始点だけを次のパスへ送る。
// 1パスで各頂点を高々1回処理するので、時間上限O(VE)を保つ。
#if defined(__GNUC__) || defined(__clang__)
[[gnu::noinline]]
#endif
inline bool BellmanFordContinue(const WGraph& graph, std::span<const int> sources,
    std::vector<std::int64_t>& distance, std::int64_t unreachable, std::int64_t lower_bound,
    std::vector<int>& negative
#ifndef SKIP_RUNTIME_CHECKS
    , std::vector<std::uint8_t>& checked
#endif
) {
    const int n = static_cast<int>(graph.size());
    std::vector<int> frontier, next;
    std::vector<std::uint8_t> scheduled(graph.size(), 0);
    for (const int from : sources) if (distance[from] != unreachable) {
        frontier.push_back(from);
        scheduled[from] = 1;
    }
    const auto process = [&](int from, auto validate) {
        for (const auto& edge : graph[from]) {
#ifndef SKIP_RUNTIME_CHECKS
            if constexpr (decltype(validate)::value) {
                constexpr std::int64_t max_cost = 1'000'000'000;
                if (edge.to < 0 || edge.to >= n)
                    throw std::out_of_range("BellmanFord: invalid destination vertex");
                if (edge.w < -max_cost || edge.w > max_cost)
                    throw std::out_of_range("BellmanFord: cost must be in [-1e9, 1e9]");
            }
#else
            (void)validate;
#endif
            const auto candidate = distance[from] + edge.w;
            if (candidate >= distance[edge.to]) continue;
            if (candidate <= lower_bound) [[unlikely]] {
                if (distance[edge.to] != lower_bound) {
                    negative.push_back(edge.to);
                    distance[edge.to] = lower_bound;
                }
            } else distance[edge.to] = candidate;
            if (!scheduled[edge.to] && !graph[edge.to].empty()) {
                next.push_back(edge.to);
                scheduled[edge.to] = 1;
            }
        }
    };
    for (int pass = 3; pass < n; ++pass) {
        for (const int from : frontier) {
            scheduled[from] = 0;
#ifndef SKIP_RUNTIME_CHECKS
            if (!checked[from]) {
                process(from, std::true_type{});
                checked[from] = 1;
            } else
#endif
                process(from, std::false_type{});
        }
        if (next.empty()) return true;
        frontier.swap(next);
        next.clear();
    }
    return false;
}

template <bool UseActive, bool Bounded = false, bool Warm = false, class... Initial>
#if defined(__GNUC__) || defined(__clang__)
[[gnu::noinline]]
#endif
inline bool BellmanFordImpl(const WGraph &graph, std::vector<std::int64_t> &distances,
                        int start_index, Initial... initial_state)
{
    const auto initial = std::tuple{initial_state...};
    auto* initial_distance = [&]() {
        if constexpr (Warm) return std::get<0>(initial);
        else return static_cast<std::vector<std::int64_t>*>(nullptr);
    }();
#ifndef SKIP_RUNTIME_CHECKS
    const int validated_until = [&]() {
        if constexpr (Warm) return std::get<1>(initial);
        else return 0;
    }();
#endif
    constexpr std::int64_t max_cost = 1'000'000'000;
    if constexpr (!Warm) {
        (void)initial_distance;
#ifndef SKIP_RUNTIME_CHECKS
        (void)validated_until;
#endif
    }
    if (graph.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        throw std::length_error("BellmanFord: too many vertices");
    const int n = static_cast<int>(graph.size());
#ifndef SKIP_RUNTIME_CHECKS
    if (start_index < 0 || start_index >= n)
        throw std::out_of_range("BellmanFord: invalid start vertex");
#endif
    if (graph[start_index].empty()) {
        distances.assign(graph.size(), INF);
        distances[start_index] = 0;
        return false;
    }
    std::vector<int> large_sources;
#ifndef SKIP_RUNTIME_CHECKS
    std::conditional_t<Warm, std::vector<std::uint8_t>, std::false_type> initial_checked;
    if constexpr (Warm) initial_checked.assign(graph.size(), 0);
#endif
#if defined(__GNUC__) && !defined(__clang__)
    alignas(64)
#endif
    std::array<int, UseActive ? 0 : 64> small_sources;
    std::size_t source_count = 0;
    if constexpr (UseActive) large_sources.reserve(std::min(graph.size(), std::size_t{64}));
    for (int from = 0; from < n; ++from)
        if (!graph[from].empty()) {
            if constexpr (UseActive) {
                // 多数の始点がある場合だけ、一度の拡張で再確保を抑える。
                if (large_sources.size() == large_sources.capacity()) large_sources.reserve(graph.size());
                large_sources.push_back(from);
#ifndef SKIP_RUNTIME_CHECKS
                if constexpr (Warm) {
                    if (from < validated_until && (*initial_distance)[from] != INF)
                        initial_checked[from] = 1;
                }
#endif
            } else small_sources[source_count++] = from;
        }
    const std::span<const int> sources = [&]() -> std::span<const int> {
        if constexpr (UseActive) return large_sources;
        else return {small_sources.data(), source_count};
    }();

    // 単純路は(n-1)辺以下。候補も[-(n+1), n+2]*max_cost内なので加算は安全。
    const auto count = static_cast<std::int64_t>(n);
#ifndef SKIP_RUNTIME_CHECKS
    constexpr bool direct_inf = Bounded;
#else
    constexpr bool direct_inf = Bounded || !UseActive;
#endif
    const std::int64_t unreachable = direct_inf ? INF : (count + 1) * max_cost;
    const std::int64_t lower_bound = -count * max_cost;
#ifndef SKIP_RUNTIME_CHECKS
    // 通常の緩和で、その頂点の出辺を全て検査したかを記録する。
    std::conditional_t<UseActive, std::vector<std::uint8_t>, std::array<std::uint8_t, 64>> checked{};
    if constexpr (UseActive) {
        if constexpr (Warm) checked.swap(initial_checked);
        else checked.assign(graph.size(), 0);
    }
#endif
    std::vector<std::int64_t> local_distance;
#if defined(__GNUC__) && !defined(__clang__)
    alignas(64)
#endif
    std::array<std::int64_t, UseActive ? 0 : 64> small_distance;
    auto& distance = [&]() -> auto& {
        if constexpr (!UseActive) return small_distance;
#ifndef SKIP_RUNTIME_CHECKS
        else return local_distance;
#else
        else return distances;
#endif
    }();
    if constexpr (UseActive) {
        if constexpr (Warm) {
            if (&distance != initial_distance) distance.swap(*initial_distance);
        } else distance.assign(graph.size(), unreachable);
    }
    else std::fill_n(distance.begin(), n, unreachable);
    std::vector<int> queue;
    distance[start_index] = 0;

    // 単純路では届かない下限を負影響の印にも使い、専用配列を省く。
    const auto mark_negative = [&](int vertex)
    {
        if (distance[vertex] == lower_bound) return;
        queue.push_back(vertex);
        distance[vertex] = lower_bound;
    };
    const auto relax_edges = [&](int from, auto validate, bool &changed)
    {
        for (const auto &edge : graph[from])
        {
#ifndef SKIP_RUNTIME_CHECKS
            if constexpr (decltype(validate)::value)
            {
                if (edge.to < 0 || edge.to >= n)
                    throw std::out_of_range("BellmanFord: invalid destination vertex");
                if (edge.w < -max_cost || edge.w > max_cost)
                    throw std::out_of_range("BellmanFord: cost must be in [-1e9, 1e9]");
            }
#else
            (void)validate;
#endif
            std::int64_t candidate = distance[from] + edge.w;
            if (candidate >= distance[edge.to])
                continue;
            // 単純路では届かない下限に達した場合だけ、負閉路の種を記録する。
            if (candidate <= lower_bound) [[unlikely]] {
                mark_negative(edge.to);
            } else {
                distance[edge.to] = candidate;
            }
            changed = true;
        }
    };
    const bool use_active = UseActive && sources.size() >= 64;
    const int pass_limit = use_active ? std::min(n, 3) : n;
    bool converged = false;
    for (int pass = 1; pass < pass_limit; ++pass)
    {
        bool changed = false;
        for (const int from : sources)
        {
            if (distance[from] == unreachable)
                continue;
#ifndef SKIP_RUNTIME_CHECKS
            if (!checked[from])
            {
                relax_edges(from, std::true_type{}, changed);
                checked[from] = 1;
            }
            else
#endif
                relax_edges(from, std::false_type{}, changed);
        }
        if (!changed)
        {
            converged = true;
            break;
        }
    }

    if constexpr (UseActive) {
        if (use_active && !converged && n > 3)
            converged = BellmanFordContinue(graph, sources, distance, unreachable, lower_bound, queue
#ifndef SKIP_RUNTIME_CHECKS
                , checked
#endif
            );
    }

    // さらに短くできる頂点を、負閉路の影響の種として記録する。
    if (!converged)
    {
        for (const int from : sources)
        {
            if (distance[from] == unreachable)
                continue;
#ifndef SKIP_RUNTIME_CHECKS
            // 最終パスで初めて到達した頂点は、ここで検査する。
            const bool validate = !checked[from];
#endif
            for (const auto &edge : graph[from])
            {
#ifndef SKIP_RUNTIME_CHECKS
                if (validate)
                {
                    if (edge.to < 0 || edge.to >= n)
                        throw std::out_of_range("BellmanFord: invalid destination vertex");
                    if (edge.w < -max_cost || edge.w > max_cost)
                        throw std::out_of_range("BellmanFord: cost must be in [-1e9, 1e9]");
                }
#endif
                if (distance[from] + edge.w < distance[edge.to])
                    mark_negative(edge.to);
            }
        }
    }
    // 到達可能な頂点の辺は検査済み。距離を加算せず、負閉路の影響を伝播する。
    for (std::size_t head = 0; head < queue.size(); ++head)
        for (const auto &edge : graph[queue[head]])
            mark_negative(edge.to);

    if constexpr (direct_inf) {
        // n*最大辺重み<INFなら有限距離はINF未満。未到達の変換と範囲走査は不要。
        for (const int vertex : queue) distance[vertex] = -INF;
    } else {
    for (int vertex = 0; vertex < n; ++vertex)
    {
        if (distance[vertex] == lower_bound)
        {
            distance[vertex] = -INF;
        }
        else if (distance[vertex] == unreachable)
        {
            distance[vertex] = INF;
        }
        else if (distance[vertex] <= -INF || distance[vertex] >= INF)
        {
            throw std::overflow_error("BellmanFord: finite distance collides with INF/-INF");
        }
    }
    }
    if constexpr (!UseActive) distances.assign(distance.begin(), distance.begin() + n);
#ifndef SKIP_RUNTIME_CHECKS
    else distances.swap(local_distance);
#endif
    return !queue.empty();
}
// 到達部分が小さければ、全頂点の出辺有無を調べずに計算する。
// 試行は64頂点・128辺まで。上限を超えた場合は通常処理に戻る。
#if defined(__GNUC__) || defined(__clang__)
[[gnu::noinline]]
#endif
inline int BellmanFordSparse(const WGraph& graph,
    std::vector<std::int64_t>& distances, int start_index) {
    constexpr std::size_t vertex_limit = 64, edge_limit = 128;
    struct CompactEdge { int from, to; std::int64_t w; };
    std::array<int, vertex_limit> vertices;
    std::array<CompactEdge, edge_limit> edges;
    std::size_t vertex_count = 1, edge_count = 0;
    vertices[0] = start_index;
    for (std::size_t head = 0; head < vertex_count; ++head) {
        for (const auto& edge : graph[vertices[head]]) {
            if (edge_count == edge_limit) return 0;
#ifndef SKIP_RUNTIME_CHECKS
            constexpr std::int64_t max_cost = 1'000'000'000;
            if (edge.to < 0 || static_cast<std::size_t>(edge.to) >= graph.size())
                throw std::out_of_range("BellmanFord: invalid destination vertex");
            if (edge.w < -max_cost || edge.w > max_cost)
                throw std::out_of_range("BellmanFord: cost must be in [-1e9, 1e9]");
#endif
            std::size_t target = 0;
            while (target < vertex_count && vertices[target] != edge.to) ++target;
            if (target == vertex_count) {
                if (vertex_count == vertex_limit) return 0;
                vertices[vertex_count++] = edge.to;
            }
            edges[edge_count++] = {static_cast<int>(head), static_cast<int>(target), edge.w};
        }
    }
    std::array<std::int64_t, vertex_limit> distance;
    std::fill_n(distance.begin(), vertex_count, INF);
    distance[0] = 0;
    // 64パス・128辺・|w|<=1e9なので、負閉路があっても加算はint64_t内。
    for (std::size_t pass = 1; pass < vertex_count; ++pass) {
        bool changed = false;
        for (std::size_t i = 0; i < edge_count; ++i) {
            const auto& edge = edges[i];
            if (distance[edge.from] == INF) continue;
            const auto candidate = distance[edge.from] + edge.w;
            if (candidate < distance[edge.to]) {
                distance[edge.to] = candidate;
                changed = true;
            }
        }
        if (!changed) break;
    }
    std::uint64_t negative = 0;
    for (std::size_t i = 0; i < edge_count; ++i) {
        const auto& edge = edges[i];
        if (distance[edge.from] != INF && distance[edge.from] + edge.w < distance[edge.to])
            negative |= std::uint64_t{1} << edge.to;
    }
    if (negative != 0) {
        for (std::size_t pass = 1; pass < vertex_count; ++pass) {
            const auto previous = negative;
            for (std::size_t i = 0; i < edge_count; ++i) {
                const auto& edge = edges[i];
                if ((negative >> edge.from) & 1) negative |= std::uint64_t{1} << edge.to;
            }
            if (negative == previous) break;
        }
        for (std::size_t i = 0; i < vertex_count; ++i)
            if ((negative >> i) & 1) distance[i] = -INF;
    }
    // 到達部分の検査・計算が全て成功してから出力を更新する。
    distances.assign(graph.size(), INF);
    for (std::size_t i = 0; i < vertex_count; ++i) distances[vertices[i]] = distance[i];
    return negative == 0 ? 1 : 2;
}

// 距離の更新が番号の増加方向だけなら、1回の緩和で距離が確定する。
// 改善する後退辺で止め、初回緩和の距離を通常処理へ渡す。
// 直前まで一本道なら祖先への改善辺は負閉路。影響を伝播して終える。
// 戻り値は確定時-1、負影響伝播完了時-2、それ以外は通常処理へ渡す停止頂点。
#if defined(__GNUC__) || defined(__clang__)
[[gnu::noinline]]
#endif
inline int BellmanFordForward(const WGraph& graph,
    std::vector<std::int64_t>& distance, int start_index) {
    distance.assign(graph.size(), INF);
    distance[start_index] = 0;
    const int n = static_cast<int>(graph.size());
    bool unique_prefix = true;
    for (int from = start_index; from < n; ++from) {
        const auto from_distance = distance[from];
        if (from_distance == INF) continue;
        for (const auto& edge : graph[from]) {
#ifndef SKIP_RUNTIME_CHECKS
            constexpr std::int64_t max_cost = 1'000'000'000;
            if (edge.to < 0 || edge.to >= n)
                throw std::out_of_range("BellmanFord: invalid destination vertex");
            if (edge.w < -max_cost || edge.w > max_cost)
                throw std::out_of_range("BellmanFord: cost must be in [-1e9, 1e9]");
#endif
            const auto candidate = from_distance + edge.w;
            if (candidate >= distance[edge.to]) continue;
            if (edge.to <= from) {
                if (!unique_prefix || distance[edge.to] == INF) return from;
                // 処理済みの到達頂点は一本道。後退先からfromへの経路が存在する。
                // candidate<distance[to]は、この閉路の重みが負であることを表す。
                std::vector<int> negative;
                negative.push_back(edge.to);
                distance[edge.to] = -INF;
                for (std::size_t head = 0; head < negative.size(); ++head) {
                    for (const auto& next : graph[negative[head]]) {
#ifndef SKIP_RUNTIME_CHECKS
                        if (next.to < 0 || next.to >= n)
                            throw std::out_of_range("BellmanFord: invalid destination vertex");
                        if (next.w < -max_cost || next.w > max_cost)
                            throw std::out_of_range("BellmanFord: cost must be in [-1e9, 1e9]");
#endif
                        if (distance[next.to] != -INF) {
                            negative.push_back(next.to);
                            distance[next.to] = -INF;
                        }
                    }
                }
                return -2;
            }
            distance[edge.to] = candidate;
        }
        // 現在の頂点で初めて分岐した場合は、この行の祖先照合まで一本道の証明が使える。
        if (graph[from].size() > 1) unique_prefix = false;
    }
    return -1;
}

} // namespace reinsirk::detail

inline bool BellmanFord(const WGraph& graph, std::vector<std::int64_t>& distances, int start_index) {
    if (graph.size() <= 64) return reinsirk::detail::BellmanFordImpl<false>(graph, distances, start_index);
    if (graph.size() >= 4096) {
        if (graph.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("BellmanFord: too many vertices");
#ifndef SKIP_RUNTIME_CHECKS
        if (start_index < 0 || static_cast<std::size_t>(start_index) >= graph.size())
            throw std::out_of_range("BellmanFord: invalid start vertex");
#endif
        const int sparse = reinsirk::detail::BellmanFordSparse(graph, distances, start_index);
        if (sparse != 0) return sparse == 2;
        if (graph.size() <= static_cast<std::size_t>((INF - 1) / 1'000'000'000) &&
            graph[start_index].size() == 1 && graph[start_index].front().to > start_index) {
#ifndef SKIP_RUNTIME_CHECKS
            std::vector<std::int64_t> initial;
#else
            auto& initial = distances;
#endif
            const int stopped = reinsirk::detail::BellmanFordForward(graph, initial, start_index);
            if (stopped < 0) {
#ifndef SKIP_RUNTIME_CHECKS
                distances.swap(initial);
#endif
                return stopped == -2;
            }
            return reinsirk::detail::BellmanFordImpl<true, true, true>(graph, distances, start_index, &initial
#ifndef SKIP_RUNTIME_CHECKS
                , stopped
#endif
            );
        }
    }
    if (graph.size() <= static_cast<std::size_t>((INF - 1) / 1'000'000'000))
        return reinsirk::detail::BellmanFordImpl<true, true>(graph, distances, start_index);
    return reinsirk::detail::BellmanFordImpl<true, false>(graph, distances, start_index);
}

#endif


// Library: Graph/Bellman-Ford_SFPA.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://zenn.dev/reputeless/books/standard-cpp-for-competitive-programming/viewer/bellman-ford

#include <array>
#include <type_traits>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#ifndef REINSIRK_INF_DEFINED
#define REINSIRK_INF_DEFINED
inline constexpr std::int64_t INF = std::int64_t{1} << 60;
#endif

#ifndef REINSIRK_GRAPH_TYPES_DEFINED
#define REINSIRK_GRAPH_TYPES_DEFINED
// 有向辺。出発点は隣接リストの添字。
struct Edge
{
    // 行先
    int to;
    // コスト
    std::int64_t w;
    Edge(int destination, std::int64_t weight) : to(destination), w(weight) {}
};
using WGraph = std::vector<std::vector<Edge>>;
#endif

// SPFA(graph, distances, start_index): 始点からの最短距離をdistancesに格納し、到達可能な負閉路があればtrueを返す。
// SKIP_RUNTIME_CHECKSを定義すると入力検査を省く。
// 検査は始点と処理中に参照する辺のみ。未到達成分は走査しない。
// 64頂点以下はキュー・補助状態をスタックに置く。
// 更新された頂点だけをFIFOキューで処理するSPFA。標準C++23、重みは[-1e9, 1e9]。
// 到達可能な負閉路があればtrue。距離は成功時に全上書きし、未到達INF・負影響-INF。
// 最悪O(VE+V+E)時間・O(V)追加領域。有限距離のINF/-INF衝突はoverflow_error。
#ifndef REINSIRK_SPFA_DEFINED
#define REINSIRK_SPFA_DEFINED
namespace reinsirk::detail {
template <bool Small, bool Bounded>
inline bool SPFARun(const WGraph &graph, std::vector<std::int64_t> &distances,
                 int start_index)
{
    constexpr std::int64_t max_cost = 1'000'000'000;
    if (graph.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        throw std::length_error("SPFA: too many vertices");
    const int n = static_cast<int>(graph.size());
#ifndef SKIP_RUNTIME_CHECKS
    if (start_index < 0 || start_index >= n)
        throw std::out_of_range("SPFA: invalid start vertex");
#endif

    // n*最大辺重み<INFなら、内部値と出力値の変換が要らない。
    const std::int64_t unreachable = Bounded ? INF : (static_cast<std::int64_t>(n) + 1) * max_cost;
    std::vector<std::int64_t> local_distance;
#ifndef SKIP_RUNTIME_CHECKS
    auto& distance = local_distance;
#else
    auto& distance = Small ? distances : local_distance;
#endif
    distance.assign(graph.size(), unreachable);
    std::conditional_t<Small, std::array<int, 64>, std::vector<int>> path_edges;
    if constexpr (Small) path_edges.fill(0);
    else path_edges.assign(graph.size(), 0);
    std::vector<int> affected;
    // stateのbit 0はキュー内、bit 1は初回検査済み、bit 2は負閉路の影響。
    constexpr std::uint8_t queued_bit = 1;
#ifndef SKIP_RUNTIME_CHECKS
    constexpr std::uint8_t checked_bit = 2;
#endif
    constexpr std::uint8_t negative_bit = 4;
    std::conditional_t<Small, std::array<std::uint8_t, 64>, std::vector<std::uint8_t>> state;
    if constexpr (Small) state.fill(0);
    else state.assign(graph.size(), 0);
    // 同時にキューに入る頂点はV個以下。書いた要素だけ読むため初期化は不要。
    const std::size_t queue_capacity = graph.size();
    std::array<int, Small ? 64 : 0> local_queue;
    std::unique_ptr<int[]> queue_owner;
    int* queue;
    if constexpr (Small) queue = local_queue.data();
    else {
        queue_owner = std::make_unique_for_overwrite<int[]>(queue_capacity);
        queue = queue_owner.get();
    }
    std::size_t head = 0, tail = 0, queued = 0;
    const auto enqueue = [&](int vertex)
    {
        queue[tail] = vertex;
        if (++tail == queue_capacity)
            tail = 0;
        ++queued;
    };
    distance[start_index] = 0;
    enqueue(start_index);
    state[start_index] = queued_bit;

    // 負閉路の下流を一度だけ走査し、以後は距離の緩和から除外する。
    const auto mark_negative = [&](int seed)
    {
        if ((state[seed] & negative_bit))
            return;
        // 負閉路が見つかるまでは、影響頂点の領域を確保しない。
        if (affected.empty())
            affected.reserve(graph.size());
        const std::size_t first = affected.size();
        state[seed] |= negative_bit;
        if constexpr (Bounded) distance[seed] = -INF;
        affected.push_back(seed);
        for (std::size_t affected_head = first; affected_head < affected.size(); ++affected_head)
        {
            for (const auto &edge : graph[affected[affected_head]])
            {
#ifndef SKIP_RUNTIME_CHECKS
                // 初回検査の途中から来る場合もあるため、伝播側でも検査する。
                if (edge.to < 0 || edge.to >= n)
                    throw std::out_of_range("SPFA: invalid destination vertex");
                if (edge.w < -max_cost || edge.w > max_cost)
                    throw std::out_of_range("SPFA: cost must be in [-1e9, 1e9]");
#endif
                if ((state[edge.to] & negative_bit))
                    continue;
                state[edge.to] |= negative_bit;
                if constexpr (Bounded) distance[edge.to] = -INF;
                affected.push_back(edge.to);
            }
        }
    };

    std::int64_t current_distance = 0;
    int current_length = 0;
    // 通常はfromが負影響ならtrue。Directでは未キューの更新先、なければ-1。
#if defined(__clang__)
    const auto relax_edge = [&]<bool Single = false, bool Direct = false>(int from, const Edge &edge)
        -> std::conditional_t<Direct, int, bool>
#else
    const auto relax_edge = [&]<bool Single = false>(int from, const Edge &edge)
#endif
    {
#if defined(__clang__)
        constexpr std::conditional_t<Direct, int, bool> unchanged = Direct ? -1 : 0;
#else
        constexpr bool unchanged = false;
#endif
        // Boundedでは候補>-INFなので、負影響先は距離比較だけで除外できる。
        if constexpr (!Bounded) if (state[edge.to] & negative_bit)
            return unchanged;
        // 格納する経路はn辺未満なので、候補の加算もint64_t内に収まる。
        const std::int64_t candidate = current_distance + edge.w;
        if (candidate >= distance[edge.to])
            return unchanged;
        const int length = current_length;
        // 複数辺では負自己ループを即時に処理し、保持した始点距離を使い続けない。
        if (length >= n || (!Single && edge.to == from)) [[unlikely]]
        {
            mark_negative(edge.to);
#if defined(__clang__)
            if constexpr (!Direct) if (state[from] & negative_bit)
                return true;
#else
            if ((state[from] & negative_bit))
                return true;
#endif
            return unchanged;
        }
        distance[edge.to] = candidate;
        path_edges[edge.to] = length;
        if (!(state[edge.to] & queued_bit))
        {
#if defined(__clang__)
            if constexpr (Direct) return edge.to;
            else
#endif
            {
                enqueue(edge.to);
                state[edge.to] |= queued_bit;
            }
        }
        return unchanged;
    };

    while (queued != 0)
    {
#if defined(__clang__)
        std::conditional_t<Small, const int, int> from = queue[head];
#else
        const int from = queue[head];
#endif
        if (++head == queue_capacity)
            head = 0;
        --queued;
#if defined(__clang__)
    process_vertex:
#endif
        const std::uint8_t vertex_state = state[from];
        if (vertex_state & negative_bit)
            continue;
#ifndef SKIP_RUNTIME_CHECKS
        state[from] = checked_bit;
#else
        state[from] = 0;
#endif
        current_distance = distance[from];
        current_length = path_edges[from] + 1;
#ifndef SKIP_RUNTIME_CHECKS
        const bool validate = !(vertex_state & checked_bit);
#endif
        // 1辺ならループと途中の自己辺更新への対処を省ける。
        if (graph[from].size() == 1) {
            const auto& edge = graph[from].front();
#ifndef SKIP_RUNTIME_CHECKS
            if (validate) {
                if (edge.to < 0 || edge.to >= n)
                    throw std::out_of_range("SPFA: invalid destination vertex");
                if (edge.w < -max_cost || edge.w > max_cost)
                    throw std::out_of_range("SPFA: cost must be in [-1e9, 1e9]");
            }
#endif
#if defined(__clang__)
            // 待機頂点がなければ直接進む。小規模は従来のコードを保つ。
            if constexpr (!Small) {
                if (queued == 0) {
                    const int next = relax_edge.template operator()<true, true>(from,edge);
                    if (next >= 0) {
                        from = next;
                        goto process_vertex;
                    }
                } else relax_edge.template operator()<true>(from,edge);
            } else
#endif
            relax_edge.template operator()<true>(from,edge);
            continue;
        }
#ifndef SKIP_RUNTIME_CHECKS
        if (validate)
        {
            for (const auto &edge : graph[from])
            {
                if (edge.to < 0 || edge.to >= n)
                    throw std::out_of_range("SPFA: invalid destination vertex");
                if (edge.w < -max_cost || edge.w > max_cost)
                    throw std::out_of_range("SPFA: cost must be in [-1e9, 1e9]");
                if (relax_edge(from, edge))
                    break;
            }
        }
        else
#endif
        {
            for (const auto &edge : graph[from])
                if (relax_edge(from, edge))
                    break;
        }
    }

    if constexpr (!Bounded) for (int vertex = 0; vertex < n; ++vertex)
    {
        if ((state[vertex] & negative_bit))
            distance[vertex] = -INF;
        else if (distance[vertex] == unreachable)
            distance[vertex] = INF;
        else if (distance[vertex] <= -INF || distance[vertex] >= INF)
            throw std::overflow_error("SPFA: finite distance collides with INF/-INF");
    }
#ifndef SKIP_RUNTIME_CHECKS
    distances.swap(distance);
#else
    if constexpr (!Small) distances.swap(distance);
#endif
    return !affected.empty();
}
} // namespace reinsirk::detail

inline bool SPFA(const WGraph& graph, std::vector<std::int64_t>& distances, int start_index) {
    if (graph.size() <= 64) return reinsirk::detail::SPFARun<true, true>(graph, distances, start_index);
    if (graph.size() <= static_cast<std::size_t>((INF - 1) / 1'000'000'000))
        return reinsirk::detail::SPFARun<false, true>(graph, distances, start_index);
    return reinsirk::detail::SPFARun<false, false>(graph, distances, start_index);
}
#endif


// Library: Graph/Dijkstra.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://zenn.dev/reputeless/books/standard-cpp-for-competitive-programming/viewer/dijkstra

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <queue>
#include <stdexcept>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef REINSIRK_INF_DEFINED
#define REINSIRK_INF_DEFINED
inline constexpr std::int64_t INF = std::int64_t{1} << 60;
#endif

#ifndef REINSIRK_GRAPH_TYPES_DEFINED
#define REINSIRK_GRAPH_TYPES_DEFINED
// 有向辺。出発点は隣接リストの添字。
struct Edge
{
    int to;
    std::int64_t w;
    Edge(int destination, std::int64_t weight) : to(destination), w(weight) {}
};
using WGraph = std::vector<std::vector<Edge>>;
#endif

// Dijkstra(graph, distances, start_index): 始点から全頂点への最短距離をdistancesに格納する。未到達はINF。
// Dijkstra(graph, distances, start_index, target_index, path): 全頂点の距離に加え、始点から終点への頂点列をpathに格納する。到達不能なら空。
// 小規模は二分ヒープ、大規模はキューが64件を超えたらradix heapへ切り替える。
// SKIP_RUNTIME_CHECKSを定義すると入力検査を省く。
// 検査は始点・終点と処理中に参照する辺のみ。未到達成分は走査しない。
#ifndef REINSIRK_DIJKSTRA_DEFINED
#define REINSIRK_DIJKSTRA_DEFINED
namespace reinsirk::detail {
// 内部用の単調整数ヒープ。格納する有限距離はINF未満。
#ifndef REINSIRK_DIJKSTRA_RADIX_HEAP_DEFINED
#define REINSIRK_DIJKSTRA_RADIX_HEAP_DEFINED
class DijkstraRadixHeap
{
    // 待機距離はlastから最大辺重み以内なので、下位32bitから復元できる。
    using State = std::pair<std::uint32_t, int>;
    std::array<std::vector<State>, 64> buckets_;
    std::vector<int> zero_;
    std::array<std::uint64_t, 64> minima_;
    std::uint64_t last_ = 0, occupied_ = 0;
#if defined(__clang__)
    [[gnu::noinline]]
#elif defined(__GNUC__)
    [[gnu::always_inline]]
#endif
    void refill()
    {
        const auto bucket = std::countr_zero(occupied_);
        auto &source = buckets_[bucket];
        last_ = minima_[bucket];
        for (const auto &state : source)
        {
            const auto key = last_ + static_cast<std::uint32_t>(state.first - static_cast<std::uint32_t>(last_));
            if (key == last_) {
                zero_.emplace_back(state.second);
                occupied_ |= 1;
                continue;
            }
            const auto target = std::bit_width(key ^ last_);
            buckets_[target].push_back(state);
            minima_[target] = std::min(minima_[target], key);
            occupied_ |= std::uint64_t{1} << target;
        }
        source.clear();
        minima_[bucket] = std::numeric_limits<std::uint64_t>::max();
        occupied_ &= ~(std::uint64_t{1} << bucket);
    }
public:
    explicit DijkstraRadixHeap(std::uint64_t last = 0) : last_(last)
    {
        minima_.fill(std::numeric_limits<std::uint64_t>::max());
    }
    bool empty() const noexcept { return occupied_ == 0; }
#if defined(__GNUC__) || defined(__clang__)
    [[gnu::always_inline]]
#endif
    void emplace(std::int64_t distance, int vertex)
    {
        const auto key = static_cast<std::uint64_t>(distance);
        if (key == last_) {
            zero_.emplace_back(vertex);
            occupied_ |= 1;
            return;
        }
        const auto bucket = std::bit_width(key ^ last_);
        buckets_[bucket].emplace_back(static_cast<std::uint32_t>(key), vertex);
        minima_[bucket] = std::min(minima_[bucket], key);
        occupied_ |= std::uint64_t{1} << bucket;
    }
#if defined(__clang__)
    [[gnu::always_inline]]
#endif
    std::pair<std::uint64_t, int> pop()
    {
        if (zero_.empty()) refill();
        const auto vertex = zero_.back();
        zero_.pop_back();
        if (zero_.empty())
            occupied_ &= ~std::uint64_t{1};
        return {last_, vertex};
    }
#if defined(__GNUC__) || defined(__clang__)
    [[gnu::always_inline]]
#endif
    std::pair<std::uint64_t, int> pop_singleton()
    {
        if (zero_.empty()) {
            const auto bucket = std::countr_zero(occupied_);
            auto& source = buckets_[bucket];
            if (source.size() == 1) {
                const int vertex = source.back().second;
                source.pop_back();
                last_ = minima_[bucket];
                minima_[bucket] = std::numeric_limits<std::uint64_t>::max();
                occupied_ &= ~(std::uint64_t{1} << bucket);
                return {last_, vertex};
            }
        }
        return pop();
    }

};


#endif

// radix移行後の末端処理。確認が64回続けて外れたら元のループへ戻す。
template <bool RestorePath, bool MayOverflow>
#if defined(__GNUC__) || defined(__clang__)
[[gnu::noinline]]
#endif
inline void DijkstraRadixRun(const WGraph& graph,
    std::vector<std::int64_t>& distance, std::vector<int>& previous,
    std::vector<int>& overflow_targets, DijkstraRadixHeap& radix,
    int first_from = -1, std::int64_t first_distance = 0, std::size_t first_begin = 0)
{
#ifndef SKIP_RUNTIME_CHECKS
    const int n = static_cast<int>(graph.size());
    constexpr std::int64_t max_weight = 1'000'000'000;
#endif
    unsigned terminal_budget = 64;
    auto* const distance_data = distance.data();
    auto* const previous_data = previous.data();
    const auto relax = [&, distance_data, previous_data]
#if defined(__clang__)
    [[gnu::always_inline]]
#endif
    (std::int64_t current, int from, std::size_t begin = 0) {
        (void)previous_data;
        const auto& edges = graph[from];
        const auto end = edges.end();
        for (auto cursor = edges.begin() + begin; cursor != end; ++cursor) {
            const auto& edge = *cursor;
#ifndef SKIP_RUNTIME_CHECKS
            if (edge.to < 0 || edge.to >= n)
                throw std::out_of_range("Dijkstra: invalid destination vertex");
            if (edge.w < 0 || edge.w > max_weight)
                throw std::out_of_range("Dijkstra: weight must be in [0, 1e9]");
#endif
            const auto candidate = current + edge.w;
            if constexpr (MayOverflow) if (candidate >= INF) [[unlikely]] {
                if (distance_data[edge.to] == INF) overflow_targets.push_back(edge.to);
                continue;
            }
            if (candidate >= distance_data[edge.to]) continue;
            distance_data[edge.to] = candidate;
            if constexpr (RestorePath) previous_data[edge.to] = from;
            if (terminal_budget != 0) {
                --terminal_budget;
                // 終端と、その1辺前だけを即時処理する。長鎖はたどらない。
                const auto& next = graph[edge.to];
                if (next.empty()) { terminal_budget = 64; continue; }
                if (next.size() == 1) {
                    const auto& tail = next.front();
#ifndef SKIP_RUNTIME_CHECKS
                    if (tail.to < 0 || tail.to >= n)
                        throw std::out_of_range("Dijkstra: invalid destination vertex");
                    if (tail.w < 0 || tail.w > max_weight)
                        throw std::out_of_range("Dijkstra: weight must be in [0, 1e9]");
#endif
                    if (graph[tail.to].empty()) {
                        terminal_budget = 64;
                        const auto terminal_distance = candidate + tail.w;
                        if constexpr (MayOverflow) if (terminal_distance >= INF) {
                            if (distance_data[tail.to] == INF) overflow_targets.push_back(tail.to);
                            continue;
                        }
                        if (terminal_distance < distance_data[tail.to]) {
                            distance_data[tail.to] = terminal_distance;
                            if constexpr (RestorePath) previous_data[tail.to] = edge.to;
                        }
                        continue;
                    }
                }
            }
        
            radix.emplace(candidate, edge.to);
        }
    };
    if (first_from >= 0) {
        relax(first_distance, first_from, first_begin);
        if (terminal_budget == 0) return;
    }
    while (!radix.empty()) {
#if defined(__GNUC__) && !defined(__clang__)
        const auto [key, vertex] = RestorePath ? radix.pop_singleton() : radix.pop();
#else
        const auto [key, vertex] = radix.pop();
#endif
        const auto value = static_cast<std::int64_t>(key);
        if (value != distance_data[vertex]) continue;
        relax(value, vertex);
        if (terminal_budget == 0) return;
    }
}

// radix移行後の所有と処理を分け、二分ヒープ側の作業領域を小さくする。
template <bool RestorePath, bool MayOverflow, class InitialRange>
#if defined(__GNUC__) || defined(__clang__)
[[gnu::noinline]]
#endif
inline void DijkstraRadixFinish(const WGraph& graph,
    std::vector<std::int64_t>& distance, std::vector<int>& previous,
    std::vector<int>& overflow_targets,
    const InitialRange& initial,
    std::int64_t current, int from, std::size_t resume)
{
#ifndef SKIP_RUNTIME_CHECKS
    const int n = static_cast<int>(graph.size());
    constexpr std::int64_t max_weight = 1'000'000'000;
#endif
    DijkstraRadixHeap radix(static_cast<std::uint64_t>(current));
    for (const auto& value : initial) radix.emplace(value.first, value.second);
    auto* const distance_data = distance.data();
    auto* const previous_data = previous.data();
    auto* const adjacency = graph.data();
    const auto relax = [&, distance_data, previous_data, adjacency]
#if defined(__GNUC__) || defined(__clang__)
    [[gnu::always_inline]]
#endif
    (std::int64_t current, int from, std::size_t begin = 0) {
        (void)previous_data;
        const auto& edges = adjacency[from];
        const auto end = edges.end();
        for (auto cursor = edges.begin() + begin; cursor != end; ++cursor)
        {
            const auto& edge = *cursor;
#ifndef SKIP_RUNTIME_CHECKS
            if (edge.to < 0 || edge.to >= n)
                throw std::out_of_range("Dijkstra: invalid destination vertex");
            if (edge.w < 0 || edge.w > max_weight)
                throw std::out_of_range("Dijkstra: weight must be in [0, 1e9]");
#endif
            const std::int64_t candidate = current + edge.w;
            if constexpr (MayOverflow) if (candidate >= INF) [[unlikely]]
            {
                if (distance_data[edge.to] == INF) overflow_targets.push_back(edge.to);
                continue;
            }
            if (candidate >= distance_data[edge.to])
                continue;
            distance_data[edge.to] = candidate;
            if constexpr (RestorePath)
                previous_data[edge.to] = from;
            radix.emplace(candidate, edge.to);
        }
    };
    if (graph.size() > 4096) {
        DijkstraRadixRun<RestorePath, MayOverflow>(
            graph, distance, previous, overflow_targets, radix, from, current, resume);
    } else if (resume < graph[from].size()) {
        relax(current, from, resume);
    }
    while (!radix.empty()) {
        const auto [key, vertex] = radix.pop();
        const auto value = static_cast<std::int64_t>(key);
        if (value == distance_data[vertex]) relax(value, vertex);
    }
}

// キューが65件になった辺でradix heapへ移し、残りの辺もそのまま処理する。
// 初期ヒープ用。未使用領域を初期化しない。
struct DijkstraInitialState {
    std::int64_t first;
    int second;
};

template <bool RestorePath, bool MayOverflow, bool Adaptive, bool PruneTerminal = true, bool RecordInitialChain = false>
#if defined(__GNUC__) || defined(__clang__)
[[gnu::always_inline]]
#endif
inline void DijkstraBigImpl(const WGraph &graph, std::vector<std::int64_t> &distances,
                         int start_index, int target_index, std::vector<int> *path)
{
    constexpr bool DeferPrevious = RestorePath && PruneTerminal && RecordInitialChain;
#if defined(__GNUC__) && !defined(__clang__)
    constexpr bool UseBounded = Adaptive && RecordInitialChain;
#else
    constexpr bool UseBounded = Adaptive;
#endif
#ifndef SKIP_RUNTIME_CHECKS
    constexpr std::int64_t max_weight = 1'000'000'000;
#endif
    if (graph.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        throw std::length_error("Dijkstra: too many vertices");
#ifndef SKIP_RUNTIME_CHECKS
    const int n = static_cast<int>(graph.size());
#endif
#ifndef SKIP_RUNTIME_CHECKS
    if (start_index < 0 || start_index >= n ||
        (RestorePath && (target_index < 0 || target_index >= n)))
        throw std::out_of_range("Dijkstra: invalid start or target vertex");
#endif

    // 有限距離をINF未満で保持。候補はINF+最大辺重み未満なので加算は安全。
    constexpr std::int64_t unreachable = INF;
#ifndef SKIP_RUNTIME_CHECKS
    std::vector<std::int64_t> local_distance;
    auto& distance = local_distance;
#else
    auto& distance = distances;
#endif
    distance.assign(graph.size(), unreachable);
    std::vector<int> overflow_targets;
    std::vector<int> previous, result;
    // 始点に出辺がなければ、経路用配列やヒープは不要。
    if constexpr (PruneTerminal) if (graph[start_index].empty()) {
        distance[start_index] = 0;
        if constexpr (RestorePath)
            if (start_index == target_index) result.push_back(start_index);
#ifndef SKIP_RUNTIME_CHECKS
        distances.swap(local_distance);
#endif
        if constexpr (RestorePath) path->swap(result);
        return;
    }
    int initial_vertex = start_index;
    std::int64_t initial_distance = 0;
    bool target_in_prefix = false;
    if constexpr (RecordInitialChain) {
        auto* const distance_data = distance.data();
        auto* const adjacency = graph.data();
        distance_data[start_index] = 0;
        result.push_back(start_index);
        target_in_prefix = start_index == target_index;
        while (adjacency[initial_vertex].size() == 1) {
            const auto& edge = adjacency[initial_vertex].front();
#ifndef SKIP_RUNTIME_CHECKS
            if (edge.to < 0 || edge.to >= n)
                throw std::out_of_range("Dijkstra: invalid destination vertex");
            if (edge.w < 0 || edge.w > max_weight)
                throw std::out_of_range("Dijkstra: weight must be in [0, 1e9]");
#endif
            const auto candidate = initial_distance + edge.w;
            if constexpr (MayOverflow) if (candidate >= INF) {
                if (distance_data[edge.to] == INF) overflow_targets.push_back(edge.to);
                break;
            }
            if (candidate >= distance_data[edge.to]) break;
            distance_data[edge.to] = candidate;
            initial_distance = candidate;
            initial_vertex = edge.to;
            if (!target_in_prefix) {
                result.push_back(initial_vertex);
                target_in_prefix = initial_vertex == target_index;
            }
        }
        // 出辺が1本以下なら未処理の到達頂点はない。
        if (adjacency[initial_vertex].size() <= 1) {
            if (!target_in_prefix) result.clear();
            if constexpr (MayOverflow) for (const int vertex : overflow_targets)
                if (distance_data[vertex] == INF)
                    throw std::overflow_error("Dijkstra: finite distance collides with INF");
#ifndef SKIP_RUNTIME_CHECKS
            distances.swap(local_distance);
#endif
            path->swap(result);
            return;
        }
    }
    if constexpr (RestorePath) {
        if constexpr (DeferPrevious) previous.reserve(graph.size());
        else previous.assign(graph.size(), -1);
    }
    // 到達範囲が小さい間は直前頂点を64件まで記録し、必要になってから全頂点分を初期化する。
    struct PreviousRecord { int vertex, parent; };
    std::array<PreviousRecord, DeferPrevious ? 64 : 0> previous_records;
    std::size_t previous_record_count = 0;
    const auto commit_previous = [&] {
        if constexpr (DeferPrevious) {
            return [&] {
                if (!previous.empty()) return;
                previous.assign(graph.size(), -1);
                for (std::size_t i = 0; i < previous_record_count; ++i) {
                    const auto& record = previous_records[i];
                    previous[record.vertex] = record.parent;
                }
            };
        } else {
            return [] {};
        }
    }();
    const auto record_previous = [&] {
        if constexpr (DeferPrevious) {
            return [&](int vertex, int parent) {
                if (previous.empty()) {
                    if (previous_record_count < previous_records.size()) {
                        previous_records[previous_record_count++] = {vertex, parent};
                        return;
                    }
                    commit_previous();
                }
                previous[vertex] = parent;
            };
        } else {
            return [](int, int) {};
        }
    }();
    using State = std::conditional_t<UseBounded, DijkstraInitialState, std::pair<std::int64_t, int>>;
    struct Compare {
        bool operator()(const State& a, const State& b) const noexcept {
            if constexpr (UseBounded)
                return a.first > b.first || (a.first == b.first && a.second > b.second);
            else return a > b;
        }
    };
    using InitialCompare = std::conditional_t<UseBounded, Compare, std::greater<State>>;
    struct InitialQueue : std::priority_queue<State, std::vector<State>, InitialCompare> {
        const std::vector<State>& states() const noexcept { return this->c; }
    };
    struct BoundedQueue {
        std::array<State, 65> storage_;
        std::size_t size_ = 0;
        bool empty() const noexcept { return size_ == 0; }
        std::size_t size() const noexcept { return size_; }
        const State& top() const noexcept { return storage_.front(); }
        void emplace(std::int64_t distance, int vertex) {
            const State value{distance, vertex};
            std::size_t hole = size_++;
            while (hole != 0) {
                const auto parent = (hole - 1) / 2;
                if (!Compare{}(storage_[parent], value)) break;
                storage_[hole] = storage_[parent];
                hole = parent;
            }
            storage_[hole] = value;
        }
        void pop() {
            const State value = storage_[--size_];
            if (size_ == 0) return;
            std::size_t hole = 0;
            for (;;) {
                auto child = hole * 2 + 1;
                if (child >= size_) break;
                if (child + 1 < size_ && Compare{}(storage_[child], storage_[child + 1])) ++child;
                if (!Compare{}(value, storage_[child])) break;
                storage_[hole] = storage_[child];
                hole = child;
            }
            storage_[hole] = value;
        }
        std::span<const State> states() const noexcept { return {storage_.data(), size_}; }
    };
    // 固定配列を選ぶ版は緩和の65件目でradixへ移るため、66件目は入らない。
    std::conditional_t<UseBounded, BoundedQueue, InitialQueue> queue;
    distance[start_index] = 0;
    queue.emplace(initial_distance, initial_vertex);

    const auto relax = [&] {
        if constexpr (!DeferPrevious) {
            return [&]<bool StopWhenLarge>
#if defined(__clang__)
    [[gnu::always_inline]]
#endif
    (auto& pending, std::int64_t current, int from, std::size_t begin = 0) -> std::size_t {
        auto* const distance_data = distance.data();
        auto* const previous_data = previous.data();
        const auto& edges = graph[from];
        const auto end = edges.end();
        for (auto cursor = edges.begin() + begin; cursor != end; ++cursor)
        {
            const auto& edge = *cursor;
#ifndef SKIP_RUNTIME_CHECKS
            if (edge.to < 0 || edge.to >= n)
                throw std::out_of_range("Dijkstra: invalid destination vertex");
            if (edge.w < 0 || edge.w > max_weight)
                throw std::out_of_range("Dijkstra: weight must be in [0, 1e9]");
#endif
            const std::int64_t candidate = current + edge.w;
            if constexpr (MayOverflow) if (candidate >= INF) [[unlikely]]
            {
                if (distance_data[edge.to] == INF) overflow_targets.push_back(edge.to);
                continue;
            }
            if (candidate >= distance_data[edge.to])
                continue;
            distance_data[edge.to] = candidate;
            if constexpr (RestorePath)
                previous_data[edge.to] = from;
            pending.emplace(candidate, edge.to);
            if constexpr (StopWhenLarge) if (pending.size() > 64) return static_cast<std::size_t>(cursor - edges.begin()) + 1;
        }
        return edges.size();
    };
        } else {
            return [&]<bool StopWhenLarge>
#if defined(__clang__)
    [[gnu::always_inline]]
#endif
    (auto& pending, std::int64_t current, int from, std::size_t begin = 0) -> std::size_t {
        auto* const distance_data = distance.data();
        auto* const previous_data = previous.data();
        const auto& edges = graph[from];
        const auto end = edges.end();
        for (auto cursor = edges.begin() + begin; cursor != end; ++cursor)
        {
            const auto& edge = *cursor;
#ifndef SKIP_RUNTIME_CHECKS
            if (edge.to < 0 || edge.to >= n)
                throw std::out_of_range("Dijkstra: invalid destination vertex");
            if (edge.w < 0 || edge.w > max_weight)
                throw std::out_of_range("Dijkstra: weight must be in [0, 1e9]");
#endif
            const std::int64_t candidate = current + edge.w;
            if constexpr (MayOverflow) if (candidate >= INF) [[unlikely]]
            {
                if (distance_data[edge.to] == INF) overflow_targets.push_back(edge.to);
                continue;
            }
            if (candidate >= distance_data[edge.to])
                continue;
            distance_data[edge.to] = candidate;
            if constexpr (RestorePath) {
                if constexpr (DeferPrevious) record_previous(edge.to, from);
                else previous_data[edge.to] = from;
            }
            pending.emplace(candidate, edge.to);
            if constexpr (StopWhenLarge) if (pending.size() > 64) return static_cast<std::size_t>(cursor - edges.begin()) + 1;
        }
        return edges.size();
    };
        }
    }();
    while (!queue.empty()) {
        auto [current, from] = queue.top(); queue.pop();
        if (current != distance[from]) continue;
        while (queue.empty() && graph[from].size() == 1) {
            const auto& edge = graph[from].front();
#ifndef SKIP_RUNTIME_CHECKS
            if (edge.to < 0 || edge.to >= n)
                throw std::out_of_range("Dijkstra: invalid destination vertex");
            if (edge.w < 0 || edge.w > max_weight)
                throw std::out_of_range("Dijkstra: weight must be in [0, 1e9]");
#endif
            const auto candidate = current + edge.w;
            if constexpr (MayOverflow) if (candidate >= INF) {
                if (distance[edge.to] == INF) overflow_targets.push_back(edge.to);
                break;
            }
            if (candidate >= distance[edge.to]) break;
            distance[edge.to] = candidate;
            if constexpr (RestorePath) {
                if constexpr (DeferPrevious) record_previous(edge.to, from);
                else previous[edge.to] = from;
            }
            current = candidate; from = edge.to;
        }
        std::size_t resume = graph[from].size();
        if (graph[from].size() != 1 || !queue.empty())
            resume = relax.template operator()<Adaptive>(queue, current, from);
        if constexpr (Adaptive) if (queue.size() > 64) {
            if constexpr (DeferPrevious) commit_previous();
            if constexpr (PruneTerminal) {
                DijkstraRadixFinish<RestorePath, MayOverflow>(
                    graph, distance, previous, overflow_targets, queue.states(), current, from, resume);
            } else {
                DijkstraRadixHeap radix(static_cast<std::uint64_t>(current));
                // 初期キューを取り出し順に並べ直す必要はない。
                for (const auto& value : queue.states())
                    radix.emplace(value.first, value.second);
                if (resume < graph[from].size())
                    relax.template operator()<false>(radix, current, from, resume);
                while (!radix.empty()) {
                    const auto [key, vertex] = radix.pop();
                    const auto value = static_cast<std::int64_t>(key);
                    if (value == distance[vertex]) relax.template operator()<false>(radix, value, vertex);
                }
            }
            break;
        }
    }

    if constexpr (RestorePath)
    {
        if constexpr (RecordInitialChain) {
            if (!target_in_prefix) {
                if (distance[target_index] == unreachable) result.clear();
                else {
                    const auto prefix_size = result.size();
                    if constexpr (!DeferPrevious) {
                        for (int vertex = target_index; vertex != initial_vertex; vertex = previous[vertex])
                            result.push_back(vertex);
                    } else if (!previous.empty()) {
                        for (int vertex = target_index; vertex != initial_vertex; vertex = previous[vertex])
                            result.push_back(vertex);
                    } else {
                        for (int vertex = target_index; vertex != initial_vertex;) {
                            result.push_back(vertex);
                            for (std::size_t i = previous_record_count; i != 0; --i) {
                                if (previous_records[i - 1].vertex == vertex) {
                                    vertex = previous_records[i - 1].parent;
                                    break;
                                }
                            }
                        }
                    }
                    std::reverse(result.begin() + static_cast<std::ptrdiff_t>(prefix_size), result.end());
                }
            }
        } else if (distance[target_index] != unreachable)
        {
            if constexpr (!DeferPrevious) {
                for (int vertex = target_index; vertex != -1; vertex = previous[vertex])
                    result.push_back(vertex);
            } else if (!previous.empty()) {
                for (int vertex = target_index; vertex != -1; vertex = previous[vertex])
                    result.push_back(vertex);
            } else {
                for (int vertex = target_index; vertex != -1;) {
                    result.push_back(vertex);
                    if (vertex == start_index) {
                        vertex = -1;
                    } else {
                        for (std::size_t i = previous_record_count; i != 0; --i) {
                            if (previous_records[i - 1].vertex == vertex) {
                                vertex = previous_records[i - 1].parent;
                                break;
                            }
                        }
                    }
                }
            }
            std::reverse(result.begin(), result.end());
        }
    }
    if constexpr (MayOverflow) for (const int vertex : overflow_targets)
        if (distance[vertex] == INF)
            throw std::overflow_error("Dijkstra: finite distance collides with INF");
#ifndef SKIP_RUNTIME_CHECKS
    distances.swap(local_distance);
#endif
    if constexpr (RestorePath)
        path->swap(result);
}

// 始点から一本道を処理する版だけを別の作業領域に展開する。
template <bool RestorePath, bool MayOverflow, bool PruneTerminal>
#if defined(__GNUC__) || defined(__clang__)
[[gnu::noinline]]
#endif
inline void DijkstraInitialChain(const WGraph& graph, std::vector<std::int64_t>& distances,
                                 int start_index, int target_index, std::vector<int>* path) {
    DijkstraBigImpl<RestorePath, MayOverflow, true, PruneTerminal, true>(
        graph, distances, start_index, target_index, path);
}

template <bool RestorePath, bool MayOverflow, bool PruneTerminal = true>
#if defined(__GNUC__) || defined(__clang__)
[[gnu::noinline]]
#endif
inline void DijkstraLarge(const WGraph& graph, std::vector<std::int64_t>& distances,
                                 int start_index, int target_index, std::vector<int>* path) {
    if constexpr (RestorePath && PruneTerminal) {
#ifndef SKIP_RUNTIME_CHECKS
        const bool valid_start = start_index >= 0 && static_cast<std::size_t>(start_index) < graph.size();
#else
        constexpr bool valid_start = true;
#endif
        if (graph.size() > 4096 && valid_start && graph[start_index].size() == 1) {
            DijkstraInitialChain<RestorePath, MayOverflow, PruneTerminal>(
                graph, distances, start_index, target_index, path);
            return;
        }
    }
    DijkstraBigImpl<RestorePath, MayOverflow, true, PruneTerminal>(graph, distances, start_index, target_index, path);
}

// 共通処理。距離のみの場合、経路用の確保と更新をコンパイル時に除く。
template <bool RestorePath>
inline void DijkstraImpl(const WGraph &graph, std::vector<std::int64_t> &distances,
                         int start_index, int target_index, std::vector<int> *path)
{
#ifndef SKIP_RUNTIME_CHECKS
    constexpr std::int64_t max_weight = 1'000'000'000;
#endif
    if (graph.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        throw std::length_error("Dijkstra: too many vertices");
#ifndef SKIP_RUNTIME_CHECKS
    const int n = static_cast<int>(graph.size());
#endif
#ifndef SKIP_RUNTIME_CHECKS
    if (start_index < 0 || start_index >= n ||
        (RestorePath && (target_index < 0 || target_index >= n)))
        throw std::out_of_range("Dijkstra: invalid start or target vertex");
#endif

    // ここは256頂点以下。全単純路がINF未満なので出力変換は不要。
    constexpr std::int64_t unreachable = INF;
#ifndef SKIP_RUNTIME_CHECKS
    std::vector<std::int64_t> local_distance;
    auto& distance = local_distance;
#else
    auto& distance = distances;
#endif
    distance.assign(graph.size(), INF);
    std::vector<int> previous, result;
    if constexpr (RestorePath)
        previous.assign(graph.size(), -1);
#if defined(__clang__)
    // 256頂点以下・重み<=1e9なら距離は38bit以内。下位8bitに頂点を詰める。
    using State = std::uint64_t;
#else
    using State = std::pair<std::int64_t, int>;
#endif
#if defined(__GNUC__) && !defined(__clang__)
    struct Compare {
        bool operator()(const State& a, const State& b) const noexcept {
            return a.first > b.first || (a.first == b.first && a.second > b.second);
        }
    };
    std::priority_queue<State, std::vector<State>, Compare> queue;
#elif defined(__clang__)
    struct Queue {
        std::vector<State> values;
        bool empty() const noexcept { return values.empty(); }
        const State& top() const noexcept { return values.front(); }
        void emplace(State value) {
            auto hole = values.size();
            values.push_back(value);
            auto* const data = values.data();
            while (hole != 0) {
                const auto parent = (hole - 1) / 2;
                if (data[parent] <= value) break;
                data[hole] = data[parent];
                hole = parent;
            }
            data[hole] = value;
        }
        void pop() {
            if (values.size() == 1) { values.pop_back(); return; }
            const auto value = values.back();
            values.pop_back();
            const auto count = values.size();
            auto* const data = values.data();
            std::size_t hole = 0;
            while (hole < count / 2) {
                auto child = hole * 2 + 1;
                if (child + 1 < count && data[child + 1] < data[child]) ++child;
                if (value <= data[child]) break;
                data[hole] = data[child];
                hole = child;
            }
            data[hole] = value;
        }
    };
    Queue queue;
#else
    std::priority_queue<State, std::vector<State>, std::greater<State>> queue;
#endif
    distance[start_index] = 0;
#if defined(__clang__)
    queue.emplace(static_cast<State>(start_index));
#else
    queue.emplace(0, start_index);
#endif

    while (!queue.empty())
    {
#if defined(__clang__)
        const auto key = queue.top();
        const auto current = static_cast<std::int64_t>(key >> 8);
        const int from = static_cast<int>(key & 255);
#else
        const auto [current, from] = queue.top();
#endif
        queue.pop();
        if (current != distance[from])
            continue;
        for (const auto &edge : graph[from])
        {
#ifndef SKIP_RUNTIME_CHECKS
            if (edge.to < 0 || edge.to >= n)
                throw std::out_of_range("Dijkstra: invalid destination vertex");
            if (edge.w < 0 || edge.w > max_weight)
                throw std::out_of_range("Dijkstra: weight must be in [0, 1e9]");
#endif
            const std::int64_t candidate = current + edge.w;
            if (candidate >= distance[edge.to])
                continue;
            distance[edge.to] = candidate;
            if constexpr (RestorePath)
                previous[edge.to] = from;
#if defined(__clang__)
            queue.emplace((static_cast<State>(candidate) << 8) | static_cast<State>(edge.to));
#else
            queue.emplace(candidate, edge.to);
#endif
        }
    }

    if constexpr (RestorePath)
    {
        if (distance[target_index] != unreachable)
        {
            for (int vertex = target_index; vertex != -1; vertex = previous[vertex])
                result.push_back(vertex);
            std::reverse(result.begin(), result.end());
        }
    }
#ifndef SKIP_RUNTIME_CHECKS
    distances.swap(local_distance);
#endif
    if constexpr (RestorePath)
        path->swap(result);
}

} // namespace reinsirk::detail

// 全頂点の最短距離を上書きする。重みは[0, 1e9]、未到達はINF。
// O(V+E log(E+1))時間・O(V+E)追加領域。有限距離がINF以上ならoverflow_error。
inline void Dijkstra(const WGraph &graph, std::vector<std::int64_t> &distances,
                     int start_index)
{
#if defined(__GNUC__) && !defined(__clang__)
    // GCCの距離版は従来のカーネルを維持する。
    constexpr bool prune_terminal = false;
#else
    constexpr bool prune_terminal = true;
#endif
    if (graph.size() <= 256) reinsirk::detail::DijkstraImpl<false>(graph, distances, start_index, -1, nullptr);
    else if (graph.size() <= 4096)
        reinsirk::detail::DijkstraLarge<false, false, false>(graph, distances, start_index, -1, nullptr);
    else if (graph.size() <= static_cast<std::size_t>((INF - 1) / 1'000'000'000))
        reinsirk::detail::DijkstraLarge<false, false, prune_terminal>(graph, distances, start_index, -1, nullptr);
    else reinsirk::detail::DijkstraLarge<false, true, prune_terminal>(graph, distances, start_index, -1, nullptr);
}

// 全頂点の距離とstart_indexからtarget_indexへの頂点列を上書きする。
// 到達不能ならpathは空。距離の仕様と計算量は3引数版と同じ。
inline void Dijkstra(const WGraph &graph, std::vector<std::int64_t> &distances,
                     int start_index, int target_index, std::vector<int> &path)
{
    if (graph.size() <= 256) reinsirk::detail::DijkstraImpl<true>(graph, distances, start_index, target_index, &path);
    else if (graph.size() <= 4096)
        reinsirk::detail::DijkstraLarge<true, false, false>(graph, distances, start_index, target_index, &path);
    else if (graph.size() <= static_cast<std::size_t>((INF - 1) / 1'000'000'000))
        reinsirk::detail::DijkstraLarge<true, false>(graph, distances, start_index, target_index, &path);
    else reinsirk::detail::DijkstraLarge<true, true>(graph, distances, start_index, target_index, &path);
}

#endif


// Library: Graph/Floyd-Warshall.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://zenn.dev/reputeless/books/standard-cpp-for-competitive-programming/viewer/floyd-warshall

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#ifndef REINSIRK_INF_DEFINED
#define REINSIRK_INF_DEFINED
inline constexpr std::int64_t INF = std::int64_t{1} << 60;
#endif

// FloydWarshall(distances): 正方行列を全点対最短距離で上書きし、グラフ内に負閉路があればtrue。O(V^3)時間。
// FloydWarshall(distances, next): 距離に加え、経路の次頂点表nextを作る。経路更新・O(V^2)追加領域はこの版のみ。
// FloydWarshallPath(next, from, to): 次頂点表から頂点列をO(経路長)で復元。未到達・負閉路の影響下なら空。
// 入力はINFが未接続、有限重みは(-INF, INF)。対角はmin(入力, 0)とする。
// 出力は未到達INF、負閉路を経由できる組だけ-INF。有限距離が(-INF, INF)外ならoverflow_error。
// 算術範囲の算出を兼ねて初期行列を走査する。SKIP_RUNTIME_CHECKSで形状・入力値・頂点の検査を除く。
// 通常版の追加領域はO(V)。大きな重みで広幅演算が必要な場合はO(V^2)。例外時の行列の内容は未規定。
#ifndef REINSIRK_FLOYD_WARSHALL_DEFINED
#define REINSIRK_FLOYD_WARSHALL_DEFINED
namespace reinsirk::detail {

#if defined(__SIZEOF_INT128__) && (defined(__GNUC__) || defined(__clang__))
using FloydWarshallWide = __int128_t;
#else
// 必要なのは最大約92ビット。符号付き上位語と符号なし下位語で標準C++でも扱う。
struct FloydWarshallWide
{
    std::int64_t high = 0;
    std::uint64_t low = 0;
    constexpr FloydWarshallWide() = default;
    constexpr FloydWarshallWide(std::int64_t value)
        : high(value < 0 ? -1 : 0), low(static_cast<std::uint64_t>(value)) {}
    constexpr FloydWarshallWide(std::int64_t upper, std::uint64_t lower)
        : high(upper), low(lower) {}
    friend constexpr FloydWarshallWide operator+(FloydWarshallWide a,
                                                        FloydWarshallWide b)
    {
        const auto lower = a.low + b.low;
        return {a.high + b.high + (lower < a.low), lower};
    }
    friend constexpr FloydWarshallWide operator-(FloydWarshallWide value)
    {
        const auto lower = std::uint64_t{0} - value.low;
        return {~value.high + (lower == 0), lower};
    }
    friend constexpr bool operator<(FloydWarshallWide a, FloydWarshallWide b)
    {
        return a.high < b.high || (a.high == b.high && a.low < b.low);
    }
    friend constexpr bool operator==(FloydWarshallWide, FloydWarshallWide) = default;
    explicit constexpr operator std::int64_t() const { return std::bit_cast<std::int64_t>(low); }
};
#endif

// 広幅実装でも下限の絶対値は単純路の最大絶対値より大きい。
inline FloydWarshallWide FloydWarshallLimit(std::size_t n)
{
#if defined(__SIZEOF_INT128__) && (defined(__GNUC__) || defined(__clang__))
    return static_cast<FloydWarshallWide>(n) * INF;
#else
    return {static_cast<std::int64_t>(n >> 4), static_cast<std::uint64_t>(n) << 60};
#endif
}

template <bool RestorePath, bool HasNegative, bool FiniteRange = false, bool Dense = false, class Distance>
inline bool FloydWarshallRun(std::vector<std::vector<Distance>> &distance,
                                    Distance unreachable, Distance lower,
                                    std::vector<std::vector<int>> *next)
{
    const std::size_t n = distance.size();
    for (std::size_t via = 0; via < n; ++via)
    {
        const auto *through = distance[via].data();
        for (std::size_t from = 0; from < n; ++from)
        {
            auto *row = distance[from].data();
            const Distance prefix = row[via];
            if (prefix == unreachable)
                continue;
            for (std::size_t to = 0; to < n; ++to)
            {
                if constexpr (!Dense) if (through[to] == unreachable)
                    continue;
                Distance candidate = prefix + through[to];
                if constexpr (HasNegative)
                    candidate = std::max(candidate, lower);
                if constexpr (RestorePath)
                {
                    if (candidate < row[to])
                    {
                        row[to] = candidate;
                        (*next)[from][to] = (*next)[from][via];
                    }
                }
                else
                    row[to] = std::min(row[to], candidate);
            }
        }
    }
    if constexpr (FiniteRange) return false;
    std::vector<std::size_t> negative;
    for (std::size_t vertex = 0; vertex < n; ++vertex)
        if (distance[vertex][vertex] < Distance{0})
            negative.push_back(vertex);
    const Distance affected = lower + Distance{-1};
    for (const std::size_t via : negative)
    {
        // 同じ強連結成分の負閉路は一度の伝播で足りる。
        if (distance[via][via] == affected)
            continue;
        for (std::size_t from = 0; from < n; ++from)
        {
            if (distance[from][via] == unreachable)
                continue;
            for (std::size_t to = 0; to < n; ++to)
                if (!(distance[via][to] == unreachable))
                {
                    distance[from][to] = affected;
                    if constexpr (RestorePath)
                        (*next)[from][to] = -1;
                }
        }
    }
    for (auto &row : distance)
        for (auto &value : row)
        {
            if (value == unreachable)
                value = Distance{INF};
            else if (value == affected)
                value = Distance{-INF};
            else if (!(Distance{-INF} < value) || !(value < Distance{INF}))
                throw std::overflow_error("FloydWarshall: finite distance collides with INF/-INF");
        }
    return !negative.empty();
}

template <bool RestorePath>
inline bool FloydWarshallImpl(std::vector<std::vector<std::int64_t>> &distance,
                                     std::vector<std::vector<int>> *next)
{
    const std::size_t n = distance.size();
    if (n > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        throw std::length_error("FloydWarshall: too many vertices");
    if constexpr (RestorePath)
        next->assign(n, std::vector<int>(n, -1));
    if (n == 0)
        return false;
    if (n == 1)
    {
#ifndef SKIP_RUNTIME_CHECKS
        if (distance[0].size() != 1)
            throw std::invalid_argument("FloydWarshall: matrix must be square");
        if (distance[0][0] != INF && (distance[0][0] <= -INF || distance[0][0] >= INF))
            throw std::out_of_range("FloydWarshall: weight must be in (-INF, INF)");
#endif
        const bool negative = distance[0][0] < 0;
        distance[0][0] = negative ? -INF : 0;
        if constexpr (RestorePath)
            (*next)[0][0] = negative ? -1 : 0;
        return negative;
    }
    std::int64_t maximum = 0;
    bool has_negative = false;
    bool dense = true;
    constexpr auto unreachable = std::numeric_limits<std::int64_t>::max();
    for (std::size_t from = 0; from < n; ++from)
    {
        auto &row = distance[from];
#ifndef SKIP_RUNTIME_CHECKS
        if (row.size() != n)
            throw std::invalid_argument("FloydWarshall: matrix must be square");
#endif
        for (std::size_t to = 0; to < n; ++to)
        {
            auto &value = row[to];
#ifndef SKIP_RUNTIME_CHECKS
            if (value != INF && (value <= -INF || value >= INF))
                throw std::out_of_range("FloydWarshall: weight must be in (-INF, INF)");
#endif
            if (from == to)
                value = std::min(value, std::int64_t{0});
            if (value == INF) {
                dense = false;
                continue;
            }
            maximum = std::max(maximum, value < 0 ? -value : value);
            has_negative = has_negative || value < 0;
            if constexpr (RestorePath)
                (*next)[from][to] = static_cast<int>(to);
        }
    }
    // 非負で最長単純路もINF未満なら、未接続値と出力の変換が要らない。
    if (!has_negative && maximum <= (INF - 1) / static_cast<std::int64_t>(n)) {
        if (dense) return FloydWarshallRun<RestorePath, false, true, true>(distance, INF, std::int64_t{0}, next);
        return FloydWarshallRun<RestorePath, false, true>(distance, INF, std::int64_t{0}, next);
    }
    for (auto& row : distance)
        for (auto& value : row)
            if (value == INF) value = unreachable;
    // 単純路はn-1辺以下。下限は-n*maximumで、負閉路での際限ない減少を抑える。
    if (maximum <= unreachable / (2 * static_cast<std::int64_t>(n)))
    {
        const auto lower = -static_cast<std::int64_t>(n) * maximum;
        if (has_negative)
            return FloydWarshallRun<RestorePath, true>(distance, unreachable, lower, next);
        return FloydWarshallRun<RestorePath, false>(distance, unreachable, lower, next);
    }

    using Wide = FloydWarshallWide;
    const Wide wide_unreachable = FloydWarshallLimit(n) + Wide{1};
    std::vector<std::vector<Wide>> wide(n, std::vector<Wide>(n));
    for (std::size_t from = 0; from < n; ++from)
        for (std::size_t to = 0; to < n; ++to)
            wide[from][to] = distance[from][to] == unreachable ? wide_unreachable : Wide{distance[from][to]};
    const bool negative = has_negative
        ? FloydWarshallRun<RestorePath, true>(wide, wide_unreachable, -FloydWarshallLimit(n), next)
        : FloydWarshallRun<RestorePath, false>(wide, wide_unreachable, -FloydWarshallLimit(n), next);
    for (std::size_t from = 0; from < n; ++from)
        for (std::size_t to = 0; to < n; ++to)
            distance[from][to] = static_cast<std::int64_t>(wide[from][to]);
    return negative;
}

} // namespace reinsirk::detail

inline bool FloydWarshall(std::vector<std::vector<std::int64_t>> &distances)
{
    return reinsirk::detail::FloydWarshallImpl<false>(distances, nullptr);
}

inline bool FloydWarshall(std::vector<std::vector<std::int64_t>> &distances,
                          std::vector<std::vector<int>> &next)
{
    return reinsirk::detail::FloydWarshallImpl<true>(distances, &next);
}

inline std::vector<int> FloydWarshallPath(const std::vector<std::vector<int>> &next,
                                        int from, int to)
{
    const std::size_t n = next.size();
#ifndef SKIP_RUNTIME_CHECKS
    if (from < 0 || to < 0 || static_cast<std::size_t>(from) >= n || static_cast<std::size_t>(to) >= n)
        throw std::out_of_range("FloydWarshallPath: invalid vertex");
#endif
    std::vector<int> path;
    const auto target = static_cast<std::size_t>(to);
    for (std::size_t steps = 0; steps <= n; ++steps)
    {
        const auto current = static_cast<std::size_t>(from);
#ifndef SKIP_RUNTIME_CHECKS
        if (next[current].size() != n)
            throw std::invalid_argument("FloydWarshallPath: table must be square");
#endif
        const int following = next[current][target];
        if (following == -1)
            return {};
        path.push_back(from);
        if (from == to)
            return path;
#ifndef SKIP_RUNTIME_CHECKS
        if (following < 0 || static_cast<std::size_t>(following) >= n)
            throw std::out_of_range("FloydWarshallPath: invalid next vertex");
#endif
        from = following;
    }
    throw std::invalid_argument("FloydWarshallPath: cyclic next table");
}
#endif


// SPDX-License-Identifier: CC0-1.0
#include <iostream>
#include <random>
#include <stdexcept>

int main() {
    const auto require=[](bool ok) { if(!ok) throw std::runtime_error("graph reference mismatch"); };
    std::mt19937 random(219381);
    for(int test=0;test<500;++test) {
        const int n=1+random()%14;
        WGraph graph(n);
        std::vector ref(n,std::vector<std::int64_t>(n,INF));
        std::vector reach(n,std::vector<bool>(n));
        for(int i=0;i<n;++i) { ref[i][i]=0;reach[i][i]=true; }
        for(int i=0;i<n*n/2;++i) {
            const int u=random()%n,v=random()%n;
            const std::int64_t w=static_cast<int>(random()%31)-10;
            graph[u].emplace_back(v,w);ref[u][v]=std::min(ref[u][v],w);reach[u][v]=true;
        }
        const auto initial=ref;
        for(int k=0;k<n;++k) for(int i=0;i<n;++i) for(int j=0;j<n;++j) {
            reach[i][j]=reach[i][j] || (reach[i][k]&&reach[k][j]);
            if(ref[i][k]!=INF && ref[k][j]!=INF) ref[i][j]=std::min(ref[i][j],ref[i][k]+ref[k][j]);
        }
        std::vector negative(n,std::vector<bool>(n));bool any=false;
        for(int k=0;k<n;++k) if(ref[k][k]<0) {
            any=true;
            for(int i=0;i<n;++i) for(int j=0;j<n;++j)
                if(reach[i][k]&&reach[k][j]) negative[i][j]=true;
        }
        auto expected=ref;
        for(int i=0;i<n;++i) for(int j=0;j<n;++j) if(negative[i][j]) expected[i][j]=-INF;
        auto distance=initial;require(FloydWarshall(distance)==any);require(distance==expected);
        std::vector<std::vector<int>> next;
        distance=initial;require(FloydWarshall(distance,next)==any);require(distance==expected);
        for(int s=0;s<n;++s) {
            const bool cycle=std::any_of(negative[s].begin(),negative[s].end(),[](bool x){return x;});
            std::vector<std::int64_t> actual;
            require(BellmanFord(graph,actual,s)==cycle);require(actual==expected[s]);
            require(SPFA(graph,actual,s)==cycle);require(actual==expected[s]);
            for(int t=0;t<n;++t) if(!negative[s][t]) {
                const auto path=FloydWarshallPath(next,s,t);
                if(!reach[s][t]) {require(path.empty());continue;}
                require(!path.empty() && path.front()==s && path.back()==t);
                std::int64_t sum=0;
                for(std::size_t i=1;i<path.size();++i) sum+=initial[path[i-1]][path[i]];
                require(sum==expected[s][t]);
            }
        }
    }
    // 256頂点をまたぎ、小規模ヒープ・大規模radix heapの両経路を使う。
    for(int n:{1,2,255,256,257,600}) {
        WGraph graph(n);
        for(int u=0;u<n;++u) {
            if(u+1<n)graph[u].emplace_back(u+1,0);
            for(int e=0;e<15;++e)graph[u].emplace_back(random()%n,random()%1'000'000'001);
        }
        for(int s:{0,n/2,n-1}) {
            std::vector<std::int64_t> expected(n,INF);expected[s]=0;
            for(int i=1;i<n;++i) {
                bool change=false;
                for(int u=0;u<n;++u) if(expected[u]!=INF) for(const auto& e:graph[u])
                    if(expected[e.to]>expected[u]+e.w) { expected[e.to]=expected[u]+e.w;change=true; }
                if(!change)break;
            }
            std::vector<std::int64_t> actual;Dijkstra(graph,actual,s);require(actual==expected);
            for(int t:{0,n/2,n-1}) {
                std::vector<int> path;Dijkstra(graph,actual,s,t,path);require(actual==expected);
                if(expected[t]==INF) {require(path.empty());continue;}
                require(path.front()==s && path.back()==t && path.size()<=static_cast<std::size_t>(n));
                std::int64_t sum=0;
                for(std::size_t i=1;i<path.size();++i) {
                    auto w=INF;
                    for(const auto& e:graph[path[i-1]]) if(e.to==path[i])w=std::min(w,e.w);
                    require(w!=INF);sum+=w;
                }
                require(sum==expected[t]);
            }
        }
    }
    std::cout<<"OK\n";
}
