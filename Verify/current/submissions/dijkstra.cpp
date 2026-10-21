#define SKIP_RUNTIME_CHECKS
// Verify: https://judge.yosupo.jp/problem/shortest_path
// Submission: https://judge.yosupo.jp/submission/411982
// Submission: https://judge.yosupo.jp/submission/411988
// Submission: https://judge.yosupo.jp/submission/411999
// Submission: https://judge.yosupo.jp/submission/412033

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


// SPDX-License-Identifier: CC0-1.0
// Verify I/O: Verify/current/common/fast_io.hpp
// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef REINSIRK_VERIFY_FAST_IO_DEFINED
#define REINSIRK_VERIFY_FAST_IO_DEFINED

// VerifyInput(file=stdin): 非所有のFILEから読む。別の入力方式と混用しない。
// >> 整数: 64bit以下の十進整数を読む。検査省略時は値域内の正しい入力を前提とする。
// >> char/string: 空白を飛ばし、1文字／空白までの文字列を読む。
// >> 浮動小数点: 十進文字列を変換する。通常のCロケールで使う。
// bool / !: 直前までの読み取りが成功しているか／失敗したかを返す。
class VerifyInput {
    static constexpr std::size_t buffer_size = 1 << 17;
    std::FILE* file_;
    std::array<unsigned char, buffer_size> buffer_;
    std::size_t position_ = 0, size_ = 0;
    bool good_ = true;

    int peek() {
        if (position_ == size_) {
            size_ = std::fread(buffer_.data(), 1, buffer_.size(), file_);
            position_ = 0;
            if (size_ == 0) return EOF;
        }
        return buffer_[position_];
    }

    int skip_space() {
        int c;
        while ((c = peek()) != EOF && c <= ' ') ++position_;
        if (c == EOF) good_ = false;
        return c;
    }

public:
    explicit VerifyInput(std::FILE* file = stdin) : file_(file) {}
    VerifyInput(const VerifyInput&) = delete;
    VerifyInput& operator=(const VerifyInput&) = delete;

    explicit operator bool() const noexcept { return good_; }
    bool operator!() const noexcept { return !good_; }

    template<class T>
        requires (std::is_integral_v<T> && sizeof(T) <= sizeof(std::uint64_t)
                  && !std::is_same_v<T, char>)
    VerifyInput& operator>>(T& result) {
        if (!good_) return *this;
        int c = skip_space();
        if (c == EOF) return *this;
        bool negative = false;
        if (c == '-' || c == '+') {
            negative = c == '-';
            ++position_;
            c = peek();
        }
        if (c == EOF) {
            good_ = false;
            return *this;
        }
#ifndef SKIP_RUNTIME_CHECKS
        if (c < '0' || c > '9') {
            good_ = false;
            return *this;
        }
        // 先頭の0を除けば、20桁以下の末尾直前の値はuint64_tに収まる。
        while (c == '0') {
            ++position_;
            c = peek();
        }
        std::uint64_t previous = 0;
        unsigned last_digit = 0;
        std::size_t digits = 0;
#endif
        std::uint64_t value = 0;
        while (c >= '0' && c <= '9') {
#ifndef SKIP_RUNTIME_CHECKS
            previous = value;
            last_digit = static_cast<unsigned>(c - '0');
            if (digits <= 20) ++digits;
#endif
            value = value * 10 + static_cast<unsigned>(c - '0');
            ++position_;
            c = peek();
        }
#ifndef SKIP_RUNTIME_CHECKS
        constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
        if ((c != EOF && c > ' ') || digits > 20
            || (digits == 20 && (previous > maximum / 10
                || (previous == maximum / 10 && last_digit > maximum % 10)))) {
            good_ = false;
            return *this;
        }
        std::uint64_t limit = static_cast<std::uint64_t>(std::numeric_limits<T>::max());
        if constexpr (std::is_signed_v<T>) limit += negative;
        else if (negative && value != 0) good_ = false;
        if (value > limit) good_ = false;
        if (!good_) return *this;
#endif
        if constexpr (std::is_signed_v<T>) {
            constexpr std::uint64_t minimum_magnitude =
                static_cast<std::uint64_t>(std::numeric_limits<T>::max()) + 1;
            // -(最小値)は評価せず、絶対値が最大値+1のときだけ最小値を使う。
            result = negative
                ? (value == minimum_magnitude ? std::numeric_limits<T>::min()
                                  : static_cast<T>(-static_cast<T>(value)))
                : static_cast<T>(value);
        } else result = static_cast<T>(value);
        return *this;
    }

    VerifyInput& operator>>(char& result) {
        if (!good_) return *this;
        const int c = skip_space();
        if (c != EOF) {
            result = static_cast<char>(c);
            ++position_;
        }
        return *this;
    }

    VerifyInput& operator>>(std::string& result) {
        if (!good_) return *this;
        int c = skip_space();
        if (c == EOF) return *this;
        result.clear();
        do {
            result.push_back(static_cast<char>(c));
            ++position_;
            c = peek();
        } while (c != EOF && c > ' ');
        return *this;
    }

    template<class T> requires std::is_floating_point_v<T>
    VerifyInput& operator>>(T& result) {
        std::string token;
        *this >> token;
        if (!good_) return *this;
#ifndef SKIP_RUNTIME_CHECKS
        char* end = nullptr;
        const long double value = std::strtold(token.c_str(), &end);
        if (end != token.c_str() + token.size()) good_ = false;
        else result = static_cast<T>(value);
#else
        result = static_cast<T>(std::strtold(token.c_str(), nullptr));
#endif
        return *this;
    }
};

// VerifyOutput(file=stdout): 非所有のFILEへ書く。破棄時にflushし、FILEは閉じない。
// << 整数/char/文字列/浮動小数点: 値を出力する。整数は64bit以下に対応する。
// fixed(digits): 浮動小数点を小数点以下digits桁で出力する（digits>=0）。
// flush(): バッファの内容をFILEへ渡す。bool / !で書き込み失敗を確認できる。
class VerifyOutput {
    static constexpr std::size_t buffer_size = 1 << 17;
    std::FILE* file_;
    std::array<char, buffer_size> buffer_;
    std::size_t size_ = 0;
    int precision_ = 6;
    bool fixed_ = false, good_ = true;

    void write(std::string_view text) {
        while (!text.empty() && good_) {
            if (size_ == buffer_.size()) flush();
            const auto count = std::min(text.size(), buffer_.size() - size_);
            std::memcpy(buffer_.data() + size_, text.data(), count);
            size_ += count;
            text.remove_prefix(count);
        }
    }

public:
    explicit VerifyOutput(std::FILE* file = stdout) : file_(file) {}
    VerifyOutput(const VerifyOutput&) = delete;
    VerifyOutput& operator=(const VerifyOutput&) = delete;
    ~VerifyOutput() { flush(); }

    explicit operator bool() const noexcept { return good_; }
    bool operator!() const noexcept { return !good_; }

    VerifyOutput& fixed(int digits) {
        if (digits < 0) throw std::invalid_argument("negative output precision");
        fixed_ = true;
        precision_ = digits;
        return *this;
    }

    void flush() noexcept {
        std::size_t written = 0;
        while (written < size_ && good_) {
            const auto count = std::fwrite(buffer_.data() + written, 1, size_ - written, file_);
            if (count == 0) good_ = false;
            written += count;
        }
        size_ = 0;
    }

    template<class T>
        requires (std::is_integral_v<T> && sizeof(T) <= sizeof(std::uint64_t)
                  && !std::is_same_v<T, char>)
    VerifyOutput& operator<<(T value) {
        if constexpr (std::is_same_v<T, bool>) return *this << static_cast<char>('0' + value);
        else {
            char digits[std::numeric_limits<std::uint64_t>::digits10 + 3];
            const auto end = std::to_chars(digits, digits + sizeof(digits), value).ptr;
            write(std::string_view(digits, static_cast<std::size_t>(end - digits)));
            return *this;
        }
    }

    VerifyOutput& operator<<(char value) {
        if (size_ == buffer_.size()) flush();
        if (good_) buffer_[size_++] = value;
        return *this;
    }

    VerifyOutput& operator<<(std::string_view value) { write(value); return *this; }
    VerifyOutput& operator<<(const std::string& value) { write(value); return *this; }
    VerifyOutput& operator<<(const char* value) { write(value); return *this; }

    template<class T> requires std::is_floating_point_v<T>
    VerifyOutput& operator<<(T value) {
        char local[128];
        const char* format = fixed_ ? "%.*Lf" : "%.*Lg";
        const int count = std::snprintf(local, sizeof(local), format, precision_,
                                        static_cast<long double>(value));
        if (count < 0) good_ = false;
        else if (static_cast<std::size_t>(count) < sizeof(local))
            write(std::string_view(local, static_cast<std::size_t>(count)));
        else {
            std::vector<char> text(static_cast<std::size_t>(count) + 1);
            std::snprintf(text.data(), text.size(), format, precision_, static_cast<long double>(value));
            write(std::string_view(text.data(), static_cast<std::size_t>(count)));
        }
        return *this;
    }
};

inline VerifyInput verify_in;
inline VerifyOutput verify_out;
#endif

int main() {
    int n{},m{},s{},t{}; if (!(verify_in>>n>>m>>s>>t)) return 0;
    WGraph graph(n);
    for (int i=0;i<m;++i) { int u{},v{}; std::int64_t w{}; verify_in>>u>>v>>w; graph[u].emplace_back(v,w); }
    std::vector<std::int64_t> distance; std::vector<int> path;
    Dijkstra(graph,distance,s,t,path);
    if (distance[t]==INF) { verify_out<<"-1\n"; return 0; }
    verify_out<<distance[t]<<' '<<path.size()-1<<'\n';
    for (std::size_t i=1;i<path.size();++i) verify_out<<path[i-1]<<' '<<path[i]<<'\n';
}
