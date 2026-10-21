// Verify: ローカル独立検査

// Library: Graph/Topological.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://zenn.dev/reputeless/books/standard-cpp-for-competitive-programming/viewer/topological-sort
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#ifndef REINSIRK_UNWEIGHTED_GRAPH_DEFINED
#define REINSIRK_UNWEIGHTED_GRAPH_DEFINED
using Graph = std::vector<std::vector<int>>;
#endif

#ifndef REINSIRK_TOPOLOGICAL_RESULT_DEFINED
#define REINSIRK_TOPOLOGICAL_RESULT_DEFINED
// order: 閉路があっても取り出せた頂点列。position: 列中の位置、未処理は-1。
// acyclic: DAGか。unique: DAGで順序が一意か（空グラフはtrue）。
struct TopologicalResult {
    std::vector<int> order;
    std::vector<int> position;
    bool acyclic = true;
    bool unique = true;
};
#endif

#ifndef REINSIRK_TOPOLOGICAL_CORE_DEFINED
#define REINSIRK_TOPOLOGICAL_CORE_DEFINED
namespace reinsirk::detail {
template <bool Lexicographical, bool Details>
inline TopologicalResult TopologicalRun(const Graph& graph) {
    const auto n = graph.size();
    if (n > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        throw std::length_error("TopologicalSort: too many vertices");
    std::vector<std::uint32_t> indegree(n);
    for (const auto& edges : graph) for (int to : edges) {
#ifndef SKIP_RUNTIME_CHECKS
        if (to < 0 || static_cast<std::size_t>(to) >= n)
            throw std::out_of_range("TopologicalSort: invalid vertex");
#endif
        auto& degree = indegree[static_cast<std::size_t>(to)];
        if (degree == std::numeric_limits<std::uint32_t>::max())
            throw std::length_error("TopologicalSort: degree overflow");
        ++degree;
    }
    TopologicalResult result;
    result.order.reserve(n);
    if constexpr (Details) result.position.assign(n, -1);
    if constexpr (!Lexicographical) {
        for (std::size_t i = 0; i < n; ++i)
            if (indegree[i] == 0) result.order.push_back(static_cast<int>(i));
        for (std::size_t head = 0; head < result.order.size(); ++head) {
            if constexpr (Details) {
                if (result.order.size() - head > 1) result.unique = false;
                result.position[static_cast<std::size_t>(result.order[head])] = static_cast<int>(head);
            }
            for (int to : graph[static_cast<std::size_t>(result.order[head])])
                if (--indegree[static_cast<std::size_t>(to)] == 0) result.order.push_back(to);
        }
    } else {
        std::vector<int> ready;
        for (std::size_t i = 0; i < n; ++i)
            if (indegree[i] == 0) ready.push_back(static_cast<int>(i));
        // 初期列は昇順なので、最小ヒープとして既に成立する。
        const std::greater<int> compare;
        while (!ready.empty()) {
            if constexpr (Details) if (ready.size() > 1) result.unique = false;
            const int from = ready.front();
            std::pop_heap(ready.begin(), ready.end(), compare);
            ready.pop_back();
            if constexpr (Details)
                result.position[static_cast<std::size_t>(from)] = static_cast<int>(result.order.size());
            result.order.push_back(from);
            for (int to : graph[static_cast<std::size_t>(from)])
                if (--indegree[static_cast<std::size_t>(to)] == 0) {
                    ready.push_back(to);
                    std::push_heap(ready.begin(), ready.end(), compare);
                }
        }
    }
    result.acyclic = result.order.size() == n;
    if constexpr (Details) result.unique = result.unique && result.acyclic;
    else if (!result.acyclic) result.order.clear();
    return result;
}
} // namespace reinsirk::detail
#endif

#ifndef REINSIRK_TOPOLOGICAL_SORT_DEFINED
#define REINSIRK_TOPOLOGICAL_SORT_DEFINED
// TopologicalSort(graph): FIFO順のトポロジカル順を返す。閉路なら空、空グラフも空。O(V+E)時間・O(V)領域。
inline std::vector<int> TopologicalSort(const Graph& graph) {
    return reinsirk::detail::TopologicalRun<false, false>(graph).order;
}
#endif
#ifndef REINSIRK_TOPOLOGICAL_SORT_LEXICOGRAPHICAL_DEFINED
#define REINSIRK_TOPOLOGICAL_SORT_LEXICOGRAPHICAL_DEFINED
// TopologicalSortLexicographical(graph): 辞書順最小のトポロジカル順。閉路なら空。O(E+V log(V+1))時間・O(V)領域。
inline std::vector<int> TopologicalSortLexicographical(const Graph& graph) {
    return reinsirk::detail::TopologicalRun<true, false>(graph).order;
}
#endif
#ifndef REINSIRK_TOPOLOGICAL_DETAILS_DEFINED
#define REINSIRK_TOPOLOGICAL_DETAILS_DEFINED
// TopologicalSortDetails(graph, lexicographical=false): 順序・各頂点の位置・DAG判定・一意性を返す。時間は対応するsortと同じ。
// IsDAG(graph): DAGかを返す。空はtrue。O(V+E)時間・O(V)領域。
// 全辺を使う入次数集計中に終点を検査し、2回目では繰り返さない。SKIPで検査を省く。
// 頂点数INT_MAX、各入次数UINT32_MAXの容量制限は常時検査する。
inline TopologicalResult TopologicalSortDetails(const Graph& graph, bool lexicographical = false) {
    return lexicographical ? reinsirk::detail::TopologicalRun<true, true>(graph)
                           : reinsirk::detail::TopologicalRun<false, true>(graph);
}
inline bool IsDAG(const Graph& graph) {
    return TopologicalSort(graph).size() == graph.size();
}
#endif


// Library: Graph/Topological_sort.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://zenn.dev/reputeless/books/standard-cpp-for-competitive-programming/viewer/topological-sort
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#ifndef REINSIRK_UNWEIGHTED_GRAPH_DEFINED
#define REINSIRK_UNWEIGHTED_GRAPH_DEFINED
using Graph = std::vector<std::vector<int>>;
#endif

#ifndef REINSIRK_TOPOLOGICAL_RESULT_DEFINED
#define REINSIRK_TOPOLOGICAL_RESULT_DEFINED
// order: 閉路があっても取り出せた頂点列。position: 列中の位置、未処理は-1。
// acyclic: DAGか。unique: DAGで順序が一意か（空グラフはtrue）。
struct TopologicalResult {
    std::vector<int> order;
    std::vector<int> position;
    bool acyclic = true;
    bool unique = true;
};
#endif

#ifndef REINSIRK_TOPOLOGICAL_CORE_DEFINED
#define REINSIRK_TOPOLOGICAL_CORE_DEFINED
namespace reinsirk::detail {
template <bool Lexicographical, bool Details>
inline TopologicalResult TopologicalRun(const Graph& graph) {
    const auto n = graph.size();
    if (n > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        throw std::length_error("TopologicalSort: too many vertices");
    std::vector<std::uint32_t> indegree(n);
    for (const auto& edges : graph) for (int to : edges) {
#ifndef SKIP_RUNTIME_CHECKS
        if (to < 0 || static_cast<std::size_t>(to) >= n)
            throw std::out_of_range("TopologicalSort: invalid vertex");
#endif
        auto& degree = indegree[static_cast<std::size_t>(to)];
        if (degree == std::numeric_limits<std::uint32_t>::max())
            throw std::length_error("TopologicalSort: degree overflow");
        ++degree;
    }
    TopologicalResult result;
    result.order.reserve(n);
    if constexpr (Details) result.position.assign(n, -1);
    if constexpr (!Lexicographical) {
        for (std::size_t i = 0; i < n; ++i)
            if (indegree[i] == 0) result.order.push_back(static_cast<int>(i));
        for (std::size_t head = 0; head < result.order.size(); ++head) {
            if constexpr (Details) {
                if (result.order.size() - head > 1) result.unique = false;
                result.position[static_cast<std::size_t>(result.order[head])] = static_cast<int>(head);
            }
            for (int to : graph[static_cast<std::size_t>(result.order[head])])
                if (--indegree[static_cast<std::size_t>(to)] == 0) result.order.push_back(to);
        }
    } else {
        std::vector<int> ready;
        for (std::size_t i = 0; i < n; ++i)
            if (indegree[i] == 0) ready.push_back(static_cast<int>(i));
        // 初期列は昇順なので、最小ヒープとして既に成立する。
        const std::greater<int> compare;
        while (!ready.empty()) {
            if constexpr (Details) if (ready.size() > 1) result.unique = false;
            const int from = ready.front();
            std::pop_heap(ready.begin(), ready.end(), compare);
            ready.pop_back();
            if constexpr (Details)
                result.position[static_cast<std::size_t>(from)] = static_cast<int>(result.order.size());
            result.order.push_back(from);
            for (int to : graph[static_cast<std::size_t>(from)])
                if (--indegree[static_cast<std::size_t>(to)] == 0) {
                    ready.push_back(to);
                    std::push_heap(ready.begin(), ready.end(), compare);
                }
        }
    }
    result.acyclic = result.order.size() == n;
    if constexpr (Details) result.unique = result.unique && result.acyclic;
    else if (!result.acyclic) result.order.clear();
    return result;
}
} // namespace reinsirk::detail
#endif

#ifndef REINSIRK_TOPOLOGICAL_SORT_DEFINED
#define REINSIRK_TOPOLOGICAL_SORT_DEFINED
// TopologicalSort(graph): FIFO順のトポロジカル順を返す。閉路なら空、空グラフも空。O(V+E)時間・O(V)領域。
inline std::vector<int> TopologicalSort(const Graph& graph) {
    return reinsirk::detail::TopologicalRun<false, false>(graph).order;
}
#endif
#ifndef REINSIRK_TOPOLOGICAL_SORT_LEXICOGRAPHICAL_DEFINED
#define REINSIRK_TOPOLOGICAL_SORT_LEXICOGRAPHICAL_DEFINED
// TopologicalSortLexicographical(graph): 辞書順最小のトポロジカル順。閉路なら空。O(E+V log(V+1))時間・O(V)領域。
inline std::vector<int> TopologicalSortLexicographical(const Graph& graph) {
    return reinsirk::detail::TopologicalRun<true, false>(graph).order;
}
#endif
#ifndef REINSIRK_TOPOLOGICAL_DETAILS_DEFINED
#define REINSIRK_TOPOLOGICAL_DETAILS_DEFINED
// TopologicalSortDetails(graph, lexicographical=false): 順序・各頂点の位置・DAG判定・一意性を返す。時間は対応するsortと同じ。
// IsDAG(graph): DAGかを返す。空はtrue。O(V+E)時間・O(V)領域。
// 全辺を使う入次数集計中に終点を検査し、2回目では繰り返さない。SKIPで検査を省く。
// 頂点数INT_MAX、各入次数UINT32_MAXの容量制限は常時検査する。
inline TopologicalResult TopologicalSortDetails(const Graph& graph, bool lexicographical = false) {
    return lexicographical ? reinsirk::detail::TopologicalRun<true, true>(graph)
                           : reinsirk::detail::TopologicalRun<false, true>(graph);
}
inline bool IsDAG(const Graph& graph) {
    return TopologicalSort(graph).size() == graph.size();
}
#endif


// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <vector>
int main(){
    auto check=[](bool ok){if(!ok)throw std::runtime_error("topological reference mismatch");};
    for(int n=0;n<=4;++n)for(unsigned mask=0;mask<(1u<<(n*n));++mask){Graph g(n);for(int i=0;i<n;++i)for(int j=0;j<n;++j)if(mask>>(i*n+j)&1)g[i].push_back(j);
        std::vector<int>p(n),first;std::iota(p.begin(),p.end(),0);int count=0;
        do{std::vector<int>pos(n);for(int i=0;i<n;++i)pos[p[i]]=i;bool valid=true;for(int i=0;i<n;++i)for(int j:g[i])if(pos[i]>=pos[j])valid=false;
            if(valid){if(count==0)first=p;++count;}}while(std::next_permutation(p.begin(),p.end()));
        check(TopologicalSortLexicographical(g)==first);check(IsDAG(g)==(count!=0));
        for(bool lex:{false,true}){auto result=TopologicalSortDetails(g,lex);check(result.acyclic==(count!=0));check(result.unique==(count==1));
            if(count){check(result.order.size()==g.size());for(std::size_t i=0;i<result.order.size();++i)check(result.position[result.order[i]]==static_cast<int>(i));
                for(int i=0;i<n;++i)for(int j:g[i])check(result.position[i]<result.position[j]);if(lex)check(result.order==first);}
        }
        auto order=TopologicalSort(g);check((order.size()==g.size())==(count!=0));if(count){std::vector<int>pos(n);for(int i=0;i<n;++i)pos[order[i]]=i;for(int i=0;i<n;++i)for(int j:g[i])check(pos[i]<pos[j]);}
    }
    std::cout<<"OK\n";
}
