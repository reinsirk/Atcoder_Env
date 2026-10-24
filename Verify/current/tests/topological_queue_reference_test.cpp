// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <iostream>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#ifndef SIRKIT_TEST_TOPOLOGICAL_SOURCE
#define SIRKIT_TEST_TOPOLOGICAL_SOURCE "../../../Graph/Topological.cpp"
#endif
#include SIRKIT_TEST_TOPOLOGICAL_SOURCE

namespace {
std::size_t cases = 0, checks = 0, many_ready_cases = 0;
void require(bool condition, const std::string& label) {
    ++checks;
    if (!condition) throw std::runtime_error(label);
}

// FIFOはdeque、辞書順はsetで候補を管理する独立参照。
TopologicalResult reference(const Graph& graph, bool lexicographical) {
    std::vector<std::size_t> degree(graph.size());
    for (const auto& edges : graph) for (int to : edges) ++degree[to];
    std::deque<int> fifo;
    std::set<int> ready;
    for (std::size_t vertex = 0; vertex < graph.size(); ++vertex) {
        if (degree[vertex] != 0) continue;
        if (lexicographical) ready.insert(static_cast<int>(vertex));
        else fifo.push_back(static_cast<int>(vertex));
    }
    TopologicalResult answer;
    answer.position.assign(graph.size(), -1);
    while (lexicographical ? !ready.empty() : !fifo.empty()) {
        if ((lexicographical ? ready.size() : fifo.size()) > 1) answer.unique = false;
        int vertex;
        if (lexicographical) {
            vertex = *ready.begin();
            ready.erase(ready.begin());
        } else {
            vertex = fifo.front();
            fifo.pop_front();
        }
        answer.position[vertex] = static_cast<int>(answer.order.size());
        answer.order.push_back(vertex);
        for (int to : graph[vertex]) {
            if (--degree[to] != 0) continue;
            if (lexicographical) ready.insert(to);
            else fifo.push_back(to);
        }
    }
    answer.acyclic = answer.order.size() == graph.size();
    answer.unique = answer.unique && answer.acyclic;
    return answer;
}

void test(const Graph& graph, const std::string& label) {
    ++cases;
    const auto original = graph;
    std::vector<std::size_t> degree(graph.size());
    for (const auto& edges : graph) for (int to : edges) ++degree[to];
    std::size_t roots = 0, degree_bits = 0;
    for (auto value : degree) { roots += value == 0; degree_bits |= value; }
    if (graph.size() >= 4096 && degree_bits > 1 && roots > graph.size() / 64 &&
        roots < graph.size() - graph.size() / 4) ++many_ready_cases;
    for (bool lexicographical : {false, true}) {
        const auto expected = reference(graph, lexicographical);
        const auto actual = TopologicalSortDetails(graph, lexicographical);
        require(actual.order == expected.order, label + ": order");
        require(actual.position == expected.position, label + ": position");
        require(actual.acyclic == expected.acyclic, label + ": acyclic");
        require(actual.unique == expected.unique, label + ": unique");
        const auto order = lexicographical ? TopologicalSortLexicographical(graph)
                                          : TopologicalSort(graph);
        require(order == (expected.acyclic ? expected.order : std::vector<int>{}),
                label + ": public sort");
        if (!lexicographical) require(IsDAG(graph) == expected.acyclic, label + ": IsDAG");
    }
    require(graph == original, label + ": input changed");
}

Graph relabel(const Graph& graph, std::mt19937& rng) {
    std::vector<int> permutation(graph.size());
    std::iota(permutation.begin(), permutation.end(), 0);
    std::shuffle(permutation.begin(), permutation.end(), rng);
    Graph answer(graph.size());
    for (std::size_t vertex = 0; vertex < graph.size(); ++vertex) {
        auto& edges = answer[permutation[vertex]];
        for (int to : graph[vertex]) edges.push_back(permutation[to]);
        std::shuffle(edges.begin(), edges.end(), rng);
    }
    return answer;
}

// roots個以外には必ず入辺を置く。重複辺の最後の減算でだけ候補になる。
Graph make_graph(int n, int roots, int kind, std::mt19937& rng) {
    Graph graph(static_cast<std::size_t>(n));
    const int limit = kind == 2 ? n - 2 : n;
    for (int vertex = roots; vertex < limit; ++vertex) {
        const int parent = kind == 0 ? (vertex == roots ? 0 : vertex - 1)
                                    : static_cast<int>(rng() % static_cast<unsigned>(vertex));
        graph[parent].push_back(vertex);
        graph[parent].push_back(vertex);
        if (kind == 1) {
            for (int repeat = 0; repeat < 3; ++repeat)
                graph[rng() % static_cast<unsigned>(vertex)].push_back(vertex);
        }
    }
    if (kind == 2) {
        graph[n - 2].push_back(n - 1);
        graph[n - 1].push_back(n - 2);
        graph[0].push_back(n - 2);
    }
    return graph;
}
} // namespace

int main() {
    try {
        std::mt19937 rng(0x71492A);
        test(Graph{}, "empty");
        test(Graph(1), "isolated");
        test(Graph{{0}}, "self loop");
        for (int n : {31, 64, 65, 4095, 4096, 4097, 16384}) {
            const int lower = n / 64, upper = n - n / 4;
            std::set<int> root_counts{1, std::max(1, lower - 1), std::max(1, lower),
                                      lower + 1, n / 3, upper - 1, upper, upper + 1, n - 2};
            for (int roots : root_counts) {
                if (roots <= 0 || roots > n - 2) continue;
                for (int kind = 0; kind < 3; ++kind) {
                    const auto graph = make_graph(n, roots, kind, rng);
                    const auto label = std::to_string(n) + "/" + std::to_string(roots) +
                                       "/" + std::to_string(kind);
                    test(graph, label);
                    test(relabel(graph, rng), label + "/permuted");
                }
            }
            test(Graph(static_cast<std::size_t>(n)), "all roots");
            Graph cycle(static_cast<std::size_t>(n));
            Graph one_incoming(static_cast<std::size_t>(n));
            for (int vertex = 0; vertex < n; ++vertex)
                cycle[vertex].push_back((vertex + 1) % n);
            for (int vertex = n / 3; vertex < n; ++vertex)
                one_incoming[vertex == n / 3 ? 0 : vertex - 1].push_back(vertex);
            test(cycle, "no roots");
            test(one_incoming, "degree bits one");
            // 候補が増えず、候補以外が全て閉路になる場合。
            Graph isolated_and_cycle(static_cast<std::size_t>(n));
            for (int vertex = n / 2; vertex < n; ++vertex) {
                const int to = vertex + 1 == n ? n / 2 : vertex + 1;
                isolated_and_cycle[vertex].push_back(to);
                isolated_and_cycle[vertex].push_back(to);
            }
            test(isolated_and_cycle, "isolated half and double cycle");
            test(relabel(isolated_and_cycle, rng), "permuted isolated half and double cycle");
            if (n >= 4096) {
                // 根の大半を取り出してから、候補幅がn/64の境界になる場合。
                for (int delta : {-1, 0, 1}) {
                    const int roots = n / 3, chain = n / 16 - 1;
                    const int width = n / 64 + delta, stop = roots + chain + width;
                    Graph narrow(static_cast<std::size_t>(n));
                    for (int vertex = roots; vertex < roots + chain; ++vertex) {
                        const int from = vertex == roots ? 0 : vertex - 1;
                        narrow[from].push_back(vertex);
                        narrow[from].push_back(vertex);
                    }
                    for (int vertex = roots + chain; vertex < stop; ++vertex) {
                        narrow[roots + chain - 1].push_back(vertex);
                        narrow[roots + chain - 1].push_back(vertex);
                    }
                    for (int vertex = stop; vertex < n; ++vertex) {
                        const int to = vertex + 1 == n ? stop : vertex + 1;
                        narrow[vertex].push_back(to);
                        narrow[vertex].push_back(to);
                    }
                    test(narrow, "pending width boundary/" + std::to_string(delta));
                    test(relabel(narrow, rng), "permuted pending width boundary/" +
                                              std::to_string(delta));
                }
            }
            // 初期候補からn/16個増える境界で処理が止まる場合。
            for (int delta : {-1, 0, 1}) {
                const int roots = n / 3;
                const int stop = roots + std::max(0, n / 16 + delta);
                Graph partial(static_cast<std::size_t>(n));
                for (int vertex = roots; vertex < stop; ++vertex) {
                    const int from = vertex == roots ? 0 : vertex - 1;
                    partial[from].push_back(vertex);
                    partial[from].push_back(vertex);
                }
                for (int vertex = stop; vertex < n; ++vertex) {
                    const int to = vertex + 1 == n ? stop : vertex + 1;
                    partial[vertex].push_back(to);
                    partial[vertex].push_back(to);
                }
                test(partial, "warm discovery boundary/" + std::to_string(delta));
                test(relabel(partial, rng), "permuted warm discovery boundary/" +
                                           std::to_string(delta));
            }
        }
        require(many_ready_cases >= 50, "insufficient many-ready coverage");
        std::cout << "{\"cases\":" << cases << ",\"checks\":" << checks
                  << ",\"many_ready_cases\":" << many_ready_cases << "}\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
