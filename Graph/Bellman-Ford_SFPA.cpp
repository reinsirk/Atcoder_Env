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

#ifndef SIRKIT_INF_DEFINED
#define SIRKIT_INF_DEFINED
inline constexpr std::int64_t INF = std::int64_t{1} << 60;
#endif

#ifndef SIRKIT_GRAPH_TYPES_DEFINED
#define SIRKIT_GRAPH_TYPES_DEFINED
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
#ifndef SIRKIT_SPFA_DEFINED
#define SIRKIT_SPFA_DEFINED
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
