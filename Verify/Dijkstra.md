https://judge.yosupo.jp/submission/411025
```
// Library Codes are in: https://github.com/reinsirk/Atcoder_Env

#include <bits/stdc++.h>
// #include <atcoder/all>
using namespace std;
// using namespace atcoder;
constexpr int64_t INF = static_cast<int64_t>(1) << 60;

template <class T>
inline bool chmin(T &a, T b);
template <class T>
inline bool chmax(T &a, T b);
// x^nを高速で計算する
inline uint64_t pow64i(uint64_t x, uint64_t n);
inline bool is_mul_overflow(int64_t a, int64_t b);
// cout << std::fixed << std::setprecision(15);

struct Edge {
    // 行先
    int to;
    // コスト
    std::int64_t w;
    Edge(int destination, std::int64_t weight)
        : to(destination), w(weight) {}
};
using Graph = std::vector<std::vector<Edge>>;

// 全頂点の最短距離と、start_indexからtarget_indexへの頂点列を求める。
// 出力は上書き。未到達はINF、経路なしは空のpath。重みは[0, 1e9]。
inline void Dijkstra(const Graph& graph,
                     std::vector<std::int64_t>& distances,
                     int start_index, int target_index,
                     std::vector<int>& path) {
    constexpr std::int64_t max_weight = 1'000'000'000;
    if (graph.size() >
        static_cast<std::size_t>(std::numeric_limits<int>::max()))
        throw std::length_error("Dijkstra: too many vertices");

    const int n = static_cast<int>(graph.size());
    if (start_index < 0 || start_index >= n ||
        target_index < 0 || target_index >= n)
        throw std::out_of_range("Dijkstra: invalid start or target vertex");

    for (const auto& edges : graph) {
        for (const auto& edge : edges) {
            if (edge.to < 0 || edge.to >= n)
                throw std::out_of_range("Dijkstra: invalid destination vertex");
            if (edge.w < 0 || edge.w > max_weight)
                throw std::out_of_range("Dijkstra: weight must be in [0, 1e9]");
        }
    }

    const std::int64_t unreachable =
        (static_cast<std::int64_t>(n) + 1) * max_weight;
    std::vector<std::int64_t> distance(graph.size(), unreachable);
    std::vector<int> previous(graph.size(), -1), result;

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
            previous[edge.to] = from;
            queue.emplace(candidate, edge.to);
        }
    }

    if (distance[target_index] != unreachable) {
        for (int v = target_index; v != -1; v = previous[v])
            result.push_back(v);
        std::reverse(result.begin(), result.end());
    }

    for (auto& value : distance) {
        if (value == unreachable) value = INF;
        else if (value >= INF)
            throw std::overflow_error("Dijkstra: finite distance collides with INF");
    }
    distances.swap(distance);
    path.swap(result);
}

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
inline uint64_t pow64i(uint64_t x, uint64_t n)
{
    uint64_t ret = 1;
    while (n > 0)
    {
        if (n & 1)
            ret *= x; // n の最下位bitが 1 ならば x^(2^i) をかける
        x *= x;
        n >>= 1; // n を1bit 左にずらす
    }
    return ret;
}
inline bool is_mul_overflow(int64_t a, int64_t b)
{
#ifdef _MSC_VER
    long long high, low = _mul128(a, b, &high);
    return !((high == 0 && low >= 0) || (high == -1 && low < 0));
#else
    long long low;
    return __builtin_smulll_overflow(a, b, &low);
#endif
}
```

https://judge.u-aizu.ac.jp/onlinejudge/review.jsp?rid=11744168#2
```
// Library Codes are in: https://github.com/reinsirk/Atcoder_Env

#include <bits/stdc++.h>
// #include <atcoder/all>
using namespace std;
// using namespace atcoder;
constexpr int64_t INF = static_cast<int64_t>(1) << 60;

template <class T>
inline bool chmin(T &a, T b);
template <class T>
inline bool chmax(T &a, T b);
// x^nを高速で計算する
inline uint64_t pow64i(uint64_t x, uint64_t n);
inline bool is_mul_overflow(int64_t a, int64_t b);
// cout << std::fixed << std::setprecision(15);

struct Edge {
    // 行先
    int to;
    // コスト
    std::int64_t w;
    Edge(int destination, std::int64_t weight)
        : to(destination), w(weight) {}
};
using Graph = std::vector<std::vector<Edge>>;

// 全頂点の最短距離と、start_indexからtarget_indexへの頂点列を求める。
// 出力は上書き。未到達はINF、経路なしは空のpath。重みは[0, 1e9]。
inline void Dijkstra(const Graph& graph,
                     std::vector<std::int64_t>& distances,
                     int start_index, int target_index,
                     std::vector<int>& path) {
    constexpr std::int64_t max_weight = 1'000'000'000;
    if (graph.size() >
        static_cast<std::size_t>(std::numeric_limits<int>::max()))
        throw std::length_error("Dijkstra: too many vertices");

    const int n = static_cast<int>(graph.size());
    if (start_index < 0 || start_index >= n ||
        target_index < 0 || target_index >= n)
        throw std::out_of_range("Dijkstra: invalid start or target vertex");

    for (const auto& edges : graph) {
        for (const auto& edge : edges) {
            if (edge.to < 0 || edge.to >= n)
                throw std::out_of_range("Dijkstra: invalid destination vertex");
            if (edge.w < 0 || edge.w > max_weight)
                throw std::out_of_range("Dijkstra: weight must be in [0, 1e9]");
        }
    }

    const std::int64_t unreachable =
        (static_cast<std::int64_t>(n) + 1) * max_weight;
    std::vector<std::int64_t> distance(graph.size(), unreachable);
    std::vector<int> previous(graph.size(), -1), result;

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
            previous[edge.to] = from;
            queue.emplace(candidate, edge.to);
        }
    }

    if (distance[target_index] != unreachable) {
        for (int v = target_index; v != -1; v = previous[v])
            result.push_back(v);
        std::reverse(result.begin(), result.end());
    }

    for (auto& value : distance) {
        if (value == unreachable) value = INF;
        else if (value >= INF)
            throw std::overflow_error("Dijkstra: finite distance collides with INF");
    }
    distances.swap(distance);
    path.swap(result);
}

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
    std::vector<int> path;
    Dijkstra(graph, distances, r, r, path);

    for (const auto distance : distances) {
        if (distance == INF) std::cout << "INF\n";
        else std::cout << distance << '\n';
    }
}
inline uint64_t pow64i(uint64_t x, uint64_t n)
{
    uint64_t ret = 1;
    while (n > 0)
    {
        if (n & 1)
            ret *= x; // n の最下位bitが 1 ならば x^(2^i) をかける
        x *= x;
        n >>= 1; // n を1bit 左にずらす
    }
    return ret;
}
inline bool is_mul_overflow(int64_t a, int64_t b)
{
#ifdef _MSC_VER
    long long high, low = _mul128(a, b, &high);
    return !((high == 0 && low >= 0) || (high == -1 && low < 0));
#else
    long long low;
    return __builtin_smulll_overflow(a, b, &low);
#endif
}
```