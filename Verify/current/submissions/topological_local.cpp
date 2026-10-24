// Verify: ローカル独立検査

// Library: Graph/Topological.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://zenn.dev/reputeless/books/standard-cpp-for-competitive-programming/viewer/topological-sort
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/topological.md
//
// API一覧
// Graph: 頂点ごとの行先を持つ無重み隣接リスト。
//
// [TopologicalResult]
// order: 閉路があっても取り出せた頂点列。position: 列中の位置、未処理は-1。
// acyclic: DAGか。unique: DAGで順序が一意か（空グラフはtrue）。
//
// [トポロジカル順序]
// TopologicalSort(graph): FIFO順のトポロジカル順を返す。閉路なら空、空グラフも空。O(V+E)時間・O(V)領域。
//
// TopologicalSortLexicographical(graph): 辞書順最小のトポロジカル順。閉路なら空。O(E+V log(V+1))時間・O(V)領域。
//
// [付加情報・DAG判定]
// TopologicalSortDetails(graph, lexicographical=false): 順序・各頂点の位置・DAG判定・一意性を返す。時間は対応するsortと同じ。
// IsDAG(graph): DAGかを返す。空はtrue。O(V+E)時間・O(V)領域。
// 全辺を使う入次数集計中に終点を検査し、2回目では繰り返さない。SKIPで検査を省く。
// 頂点数INT_MAX、各入次数UINT32_MAXの容量制限は常時検査する。

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))
#include <immintrin.h>
#endif

#ifndef SIRKIT_UNWEIGHTED_GRAPH_DEFINED
#define SIRKIT_UNWEIGHTED_GRAPH_DEFINED
using Graph = std::vector<std::vector<int>>;
#endif

#ifndef SIRKIT_TOPOLOGICAL_RESULT_DEFINED
#define SIRKIT_TOPOLOGICAL_RESULT_DEFINED
struct TopologicalResult {
    std::vector<int> order;
    std::vector<int> position;
    bool acyclic = true;
    bool unique = true;
};
#endif

#ifndef SIRKIT_TOPOLOGICAL_READY_MASK_DEFINED
#define SIRKIT_TOPOLOGICAL_READY_MASK_DEFINED
namespace reinsirk::detail {
inline std::uint64_t TopologicalReadyMask(const std::uint32_t* degree) {
    std::uint64_t mask = 0;
    for (unsigned i = 0; i < 64; ++i)
        mask |= static_cast<std::uint64_t>(degree[i] == 0) << i;
    return mask;
}
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))
__attribute__((target("avx2")))
inline std::uint64_t TopologicalReadyMaskAvx2(const std::uint32_t* degree) {
    const auto zero = _mm256_setzero_si256();
    std::uint64_t mask = 0;
    for (unsigned i = 0; i < 64; i += 8) {
        const auto values = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(degree + i));
        const auto equal = _mm256_cmpeq_epi32(values, zero);
        const auto byte = static_cast<unsigned>(_mm256_movemask_ps(_mm256_castsi256_ps(equal)));
        mask |= static_cast<std::uint64_t>(byte) << i;
    }
    return mask;
}
#endif
} // namespace reinsirk::detail
#endif

#ifndef SIRKIT_TOPOLOGICAL_CORE_DEFINED
#define SIRKIT_TOPOLOGICAL_CORE_DEFINED
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
        // 各頂点は高々1回追加し、reserve(n)済みなので容量を超えない。
        const auto append = [&](int vertex) {
            if (result.order.size() == result.order.capacity()) std::unreachable();
            result.order.push_back(vertex);
        };
        std::uint32_t degree_bits = 0;
        for (std::size_t i = 0; i < n; ++i) {
            degree_bits |= indegree[i];
            if (indegree[i] == 0) append(static_cast<int>(i));
        }
        if (n >= 4096 && degree_bits > 1 && result.order.size() > n / 64 && result.order.size() < n - n / 4) {
            // 到達できる頂点が十分増えてから、添字式キューに切り替える。
            const auto switch_size = result.order.size() + n / 16;
            std::size_t head = 0;
            while (head < result.order.size() &&
                   (result.order.size() < switch_size || result.order.size() - head <= n / 64)) {
                if constexpr (Details) {
                    if (result.order.size() - head > 1) result.unique = false;
                    result.position[static_cast<std::size_t>(result.order[head])] = static_cast<int>(head);
                }
                for (int to : graph[static_cast<std::size_t>(result.order[head])])
                    if (--indegree[static_cast<std::size_t>(to)] == 0) append(to);
                ++head;
            }
            if (head < result.order.size()) {
                std::size_t tail = result.order.size();
                result.order.resize(n);
                for (; head < tail; ++head) {
                    if constexpr (Details) {
                        if (tail - head > 1) result.unique = false;
                        result.position[static_cast<std::size_t>(result.order[head])] = static_cast<int>(head);
                    }
                    for (int to : graph[static_cast<std::size_t>(result.order[head])]) {
                        const bool ready = --indegree[static_cast<std::size_t>(to)] == 0;
                        // 全頂点が入隊済みなら未処理の辺はなく、常にtail < n。
                        result.order[tail] = to;
                        tail += ready;
                    }
                }
                result.order.resize(tail);
            }
        } else {
            for (std::size_t head = 0; head < result.order.size(); ++head) {
                if constexpr (Details) {
                    if (result.order.size() - head > 1) result.unique = false;
                    result.position[static_cast<std::size_t>(result.order[head])] = static_cast<int>(head);
                }
                for (int to : graph[static_cast<std::size_t>(result.order[head])])
                    if (--indegree[static_cast<std::size_t>(to)] == 0) append(to);
            }
        }
    } else {
        std::array<std::size_t, 6> offsets{};
        std::size_t words = (n + 63) / 64, total = 0, levels = 0;
        // 頂点数はINT_MAX以下なので、64進の階層は高々6段。
        do {
            offsets[levels++] = total;
            total += words;
            if (words <= 1) break;
            words = (words + 63) / 64;
        } while (true);
        std::uint64_t single_word = 0;
        std::vector<std::uint64_t> large_bits;
        auto* bits = n <= 64 ? &single_word : nullptr;
        std::size_t ready_count = 0;
        // 最小値を別に保持し、候補1個の間はビット集合を使わない。
        int minimum = -1;
        const auto insert = [&](int vertex) {
            ++ready_count;
            if (minimum < 0) { minimum = vertex; return; }
            if (vertex < minimum) std::swap(vertex, minimum);
            if (bits == nullptr) {
                // 複数候補が初めて現れたときだけ確保する。
                large_bits.assign(total, 0);
                bits = large_bits.data();
            }
            std::size_t index = static_cast<std::size_t>(vertex);
            for (std::size_t level = 0; level < levels; ++level) {
                auto& word = bits[offsets[level] + (index >> 6)];
                const auto old = word;
                word |= std::uint64_t{1} << (index & 63);
                if (old != 0) break;
                index >>= 6;
            }
        };
        auto make_mask = &TopologicalReadyMask;
        bool bulk_initialize = false;
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))
        if (n >= 4096 && __builtin_cpu_supports("avx2")) {
            bulk_initialize = true;
            make_mask = &TopologicalReadyMaskAvx2;
        }
#endif
        if (bulk_initialize) {
            for (std::size_t base = 0; base < n; base += 64) {
                std::uint64_t mask = 0;
                if (n - base >= 64) mask = make_mask(indegree.data() + base);
                else for (std::size_t i = base; i < n; ++i)
                    mask |= static_cast<std::uint64_t>(indegree[i] == 0) << (i - base);
                ready_count += std::popcount(mask);
                if (mask != 0 && minimum < 0) {
                    minimum = static_cast<int>(base + std::countr_zero(mask));
                    mask &= mask - 1;
                }
                if (mask != 0) {
                    if (bits == nullptr) {
                        large_bits.assign(total, 0);
                        bits = large_bits.data();
                    }
                    bits[base >> 6] = mask;
                }
            }
            if (ready_count > 1) {
                std::size_t count = (n + 63) / 64;
                for (std::size_t level = 0; level + 1 < levels; ++level) {
                    for (std::size_t i = 0; i < count; ++i)
                        if (bits[offsets[level] + i] != 0)
                            bits[offsets[level + 1] + (i >> 6)] |= std::uint64_t{1} << (i & 63);
                    count = (count + 63) / 64;
                }
            }
        } else {
            for (std::size_t i = 0; i < n; ++i)
                if (indegree[i] == 0) insert(static_cast<int>(i));
        }
        while (ready_count != 0) {
            if constexpr (Details) if (ready_count > 1) result.unique = false;
            const int from = minimum;
            minimum = -1;
            if (--ready_count != 0) {
                std::size_t index = static_cast<std::size_t>(from) >> 6;
                if (bits[index] != 0) {
                    index = (index << 6) + std::countr_zero(bits[index]);
                } else {
                    index = 0;
                    for (std::size_t level = levels; level-- != 0;)
                        index = (index << 6) + std::countr_zero(bits[offsets[level] + index]);
                }
                minimum = static_cast<int>(index);
                for (std::size_t level = 0; level < levels; ++level) {
                    auto& word = bits[offsets[level] + (index >> 6)];
                    word &= ~(std::uint64_t{1} << (index & 63));
                    if (word != 0) break;
                    index >>= 6;
                }
            }
            if constexpr (Details)
                result.position[static_cast<std::size_t>(from)] = static_cast<int>(result.order.size());
            result.order.push_back(from);
            for (int to : graph[static_cast<std::size_t>(from)])
                if (--indegree[static_cast<std::size_t>(to)] == 0) insert(to);
        }
    }
    result.acyclic = result.order.size() == n;
    if constexpr (Details) result.unique = result.unique && result.acyclic;
    else if (!result.acyclic) result.order.clear();
    return result;
}
} // namespace reinsirk::detail
#endif

#ifndef SIRKIT_TOPOLOGICAL_SORT_DEFINED
#define SIRKIT_TOPOLOGICAL_SORT_DEFINED
inline std::vector<int> TopologicalSort(const Graph& graph) {
    return reinsirk::detail::TopologicalRun<false, false>(graph).order;
}
#endif
#ifndef SIRKIT_TOPOLOGICAL_SORT_LEXICOGRAPHICAL_DEFINED
#define SIRKIT_TOPOLOGICAL_SORT_LEXICOGRAPHICAL_DEFINED
inline std::vector<int> TopologicalSortLexicographical(const Graph& graph) {
    return reinsirk::detail::TopologicalRun<true, false>(graph).order;
}
#endif
#ifndef SIRKIT_TOPOLOGICAL_DETAILS_DEFINED
#define SIRKIT_TOPOLOGICAL_DETAILS_DEFINED
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
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/topological.md
//
// API一覧
// Graph: 頂点ごとの行先を持つ無重み隣接リスト。
//
// [TopologicalResult]
// order: 閉路があっても取り出せた頂点列。position: 列中の位置、未処理は-1。
// acyclic: DAGか。unique: DAGで順序が一意か（空グラフはtrue）。
//
// [トポロジカル順序]
// TopologicalSort(graph): FIFO順のトポロジカル順を返す。閉路なら空、空グラフも空。O(V+E)時間・O(V)領域。
//
// TopologicalSortLexicographical(graph): 辞書順最小のトポロジカル順。閉路なら空。O(E+V log(V+1))時間・O(V)領域。
//
// [付加情報・DAG判定]
// TopologicalSortDetails(graph, lexicographical=false): 順序・各頂点の位置・DAG判定・一意性を返す。時間は対応するsortと同じ。
// IsDAG(graph): DAGかを返す。空はtrue。O(V+E)時間・O(V)領域。
// 全辺を使う入次数集計中に終点を検査し、2回目では繰り返さない。SKIPで検査を省く。
// 頂点数INT_MAX、各入次数UINT32_MAXの容量制限は常時検査する。

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))
#include <immintrin.h>
#endif

#ifndef SIRKIT_UNWEIGHTED_GRAPH_DEFINED
#define SIRKIT_UNWEIGHTED_GRAPH_DEFINED
using Graph = std::vector<std::vector<int>>;
#endif

#ifndef SIRKIT_TOPOLOGICAL_RESULT_DEFINED
#define SIRKIT_TOPOLOGICAL_RESULT_DEFINED
struct TopologicalResult {
    std::vector<int> order;
    std::vector<int> position;
    bool acyclic = true;
    bool unique = true;
};
#endif

#ifndef SIRKIT_TOPOLOGICAL_READY_MASK_DEFINED
#define SIRKIT_TOPOLOGICAL_READY_MASK_DEFINED
namespace reinsirk::detail {
inline std::uint64_t TopologicalReadyMask(const std::uint32_t* degree) {
    std::uint64_t mask = 0;
    for (unsigned i = 0; i < 64; ++i)
        mask |= static_cast<std::uint64_t>(degree[i] == 0) << i;
    return mask;
}
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))
__attribute__((target("avx2")))
inline std::uint64_t TopologicalReadyMaskAvx2(const std::uint32_t* degree) {
    const auto zero = _mm256_setzero_si256();
    std::uint64_t mask = 0;
    for (unsigned i = 0; i < 64; i += 8) {
        const auto values = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(degree + i));
        const auto equal = _mm256_cmpeq_epi32(values, zero);
        const auto byte = static_cast<unsigned>(_mm256_movemask_ps(_mm256_castsi256_ps(equal)));
        mask |= static_cast<std::uint64_t>(byte) << i;
    }
    return mask;
}
#endif
} // namespace reinsirk::detail
#endif

#ifndef SIRKIT_TOPOLOGICAL_CORE_DEFINED
#define SIRKIT_TOPOLOGICAL_CORE_DEFINED
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
        // 各頂点は高々1回追加し、reserve(n)済みなので容量を超えない。
        const auto append = [&](int vertex) {
            if (result.order.size() == result.order.capacity()) std::unreachable();
            result.order.push_back(vertex);
        };
        std::uint32_t degree_bits = 0;
        for (std::size_t i = 0; i < n; ++i) {
            degree_bits |= indegree[i];
            if (indegree[i] == 0) append(static_cast<int>(i));
        }
        if (n >= 4096 && degree_bits > 1 && result.order.size() > n / 64 && result.order.size() < n - n / 4) {
            // 到達できる頂点が十分増えてから、添字式キューに切り替える。
            const auto switch_size = result.order.size() + n / 16;
            std::size_t head = 0;
            while (head < result.order.size() &&
                   (result.order.size() < switch_size || result.order.size() - head <= n / 64)) {
                if constexpr (Details) {
                    if (result.order.size() - head > 1) result.unique = false;
                    result.position[static_cast<std::size_t>(result.order[head])] = static_cast<int>(head);
                }
                for (int to : graph[static_cast<std::size_t>(result.order[head])])
                    if (--indegree[static_cast<std::size_t>(to)] == 0) append(to);
                ++head;
            }
            if (head < result.order.size()) {
                std::size_t tail = result.order.size();
                result.order.resize(n);
                for (; head < tail; ++head) {
                    if constexpr (Details) {
                        if (tail - head > 1) result.unique = false;
                        result.position[static_cast<std::size_t>(result.order[head])] = static_cast<int>(head);
                    }
                    for (int to : graph[static_cast<std::size_t>(result.order[head])]) {
                        const bool ready = --indegree[static_cast<std::size_t>(to)] == 0;
                        // 全頂点が入隊済みなら未処理の辺はなく、常にtail < n。
                        result.order[tail] = to;
                        tail += ready;
                    }
                }
                result.order.resize(tail);
            }
        } else {
            for (std::size_t head = 0; head < result.order.size(); ++head) {
                if constexpr (Details) {
                    if (result.order.size() - head > 1) result.unique = false;
                    result.position[static_cast<std::size_t>(result.order[head])] = static_cast<int>(head);
                }
                for (int to : graph[static_cast<std::size_t>(result.order[head])])
                    if (--indegree[static_cast<std::size_t>(to)] == 0) append(to);
            }
        }
    } else {
        std::array<std::size_t, 6> offsets{};
        std::size_t words = (n + 63) / 64, total = 0, levels = 0;
        // 頂点数はINT_MAX以下なので、64進の階層は高々6段。
        do {
            offsets[levels++] = total;
            total += words;
            if (words <= 1) break;
            words = (words + 63) / 64;
        } while (true);
        std::uint64_t single_word = 0;
        std::vector<std::uint64_t> large_bits;
        auto* bits = n <= 64 ? &single_word : nullptr;
        std::size_t ready_count = 0;
        // 最小値を別に保持し、候補1個の間はビット集合を使わない。
        int minimum = -1;
        const auto insert = [&](int vertex) {
            ++ready_count;
            if (minimum < 0) { minimum = vertex; return; }
            if (vertex < minimum) std::swap(vertex, minimum);
            if (bits == nullptr) {
                // 複数候補が初めて現れたときだけ確保する。
                large_bits.assign(total, 0);
                bits = large_bits.data();
            }
            std::size_t index = static_cast<std::size_t>(vertex);
            for (std::size_t level = 0; level < levels; ++level) {
                auto& word = bits[offsets[level] + (index >> 6)];
                const auto old = word;
                word |= std::uint64_t{1} << (index & 63);
                if (old != 0) break;
                index >>= 6;
            }
        };
        auto make_mask = &TopologicalReadyMask;
        bool bulk_initialize = false;
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))
        if (n >= 4096 && __builtin_cpu_supports("avx2")) {
            bulk_initialize = true;
            make_mask = &TopologicalReadyMaskAvx2;
        }
#endif
        if (bulk_initialize) {
            for (std::size_t base = 0; base < n; base += 64) {
                std::uint64_t mask = 0;
                if (n - base >= 64) mask = make_mask(indegree.data() + base);
                else for (std::size_t i = base; i < n; ++i)
                    mask |= static_cast<std::uint64_t>(indegree[i] == 0) << (i - base);
                ready_count += std::popcount(mask);
                if (mask != 0 && minimum < 0) {
                    minimum = static_cast<int>(base + std::countr_zero(mask));
                    mask &= mask - 1;
                }
                if (mask != 0) {
                    if (bits == nullptr) {
                        large_bits.assign(total, 0);
                        bits = large_bits.data();
                    }
                    bits[base >> 6] = mask;
                }
            }
            if (ready_count > 1) {
                std::size_t count = (n + 63) / 64;
                for (std::size_t level = 0; level + 1 < levels; ++level) {
                    for (std::size_t i = 0; i < count; ++i)
                        if (bits[offsets[level] + i] != 0)
                            bits[offsets[level + 1] + (i >> 6)] |= std::uint64_t{1} << (i & 63);
                    count = (count + 63) / 64;
                }
            }
        } else {
            for (std::size_t i = 0; i < n; ++i)
                if (indegree[i] == 0) insert(static_cast<int>(i));
        }
        while (ready_count != 0) {
            if constexpr (Details) if (ready_count > 1) result.unique = false;
            const int from = minimum;
            minimum = -1;
            if (--ready_count != 0) {
                std::size_t index = static_cast<std::size_t>(from) >> 6;
                if (bits[index] != 0) {
                    index = (index << 6) + std::countr_zero(bits[index]);
                } else {
                    index = 0;
                    for (std::size_t level = levels; level-- != 0;)
                        index = (index << 6) + std::countr_zero(bits[offsets[level] + index]);
                }
                minimum = static_cast<int>(index);
                for (std::size_t level = 0; level < levels; ++level) {
                    auto& word = bits[offsets[level] + (index >> 6)];
                    word &= ~(std::uint64_t{1} << (index & 63));
                    if (word != 0) break;
                    index >>= 6;
                }
            }
            if constexpr (Details)
                result.position[static_cast<std::size_t>(from)] = static_cast<int>(result.order.size());
            result.order.push_back(from);
            for (int to : graph[static_cast<std::size_t>(from)])
                if (--indegree[static_cast<std::size_t>(to)] == 0) insert(to);
        }
    }
    result.acyclic = result.order.size() == n;
    if constexpr (Details) result.unique = result.unique && result.acyclic;
    else if (!result.acyclic) result.order.clear();
    return result;
}
} // namespace reinsirk::detail
#endif

#ifndef SIRKIT_TOPOLOGICAL_SORT_DEFINED
#define SIRKIT_TOPOLOGICAL_SORT_DEFINED
inline std::vector<int> TopologicalSort(const Graph& graph) {
    return reinsirk::detail::TopologicalRun<false, false>(graph).order;
}
#endif
#ifndef SIRKIT_TOPOLOGICAL_SORT_LEXICOGRAPHICAL_DEFINED
#define SIRKIT_TOPOLOGICAL_SORT_LEXICOGRAPHICAL_DEFINED
inline std::vector<int> TopologicalSortLexicographical(const Graph& graph) {
    return reinsirk::detail::TopologicalRun<true, false>(graph).order;
}
#endif
#ifndef SIRKIT_TOPOLOGICAL_DETAILS_DEFINED
#define SIRKIT_TOPOLOGICAL_DETAILS_DEFINED
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
