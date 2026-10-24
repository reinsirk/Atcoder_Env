// SPDX-License-Identifier: CC0-1.0
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
#ifndef SIRKIT_TEST_NEGATIVE_PATH_SOURCE
#define SIRKIT_TEST_NEGATIVE_PATH_SOURCE "../../../Graph/Bellman-Ford.cpp"
#endif
#ifndef SIRKIT_TEST_NEGATIVE_PATH_API
#define SIRKIT_TEST_NEGATIVE_PATH_API BellmanFord
#endif
#include SIRKIT_TEST_NEGATIVE_PATH_SOURCE
static bool fail_allocations = false;
static int remaining_allocations = 0;
static std::size_t live_allocations = 0;
// 置換allocatorをインライン化せず、GCCの誤ったnew/free不一致診断を避ける。
#if defined(__GNUC__) || defined(__clang__)
#define SIRKIT_TEST_NOINLINE [[gnu::noinline]]
#else
#define SIRKIT_TEST_NOINLINE
#endif
SIRKIT_TEST_NOINLINE void* operator new(std::size_t size) {
    if (fail_allocations && remaining_allocations-- == 0) throw std::bad_alloc();
    void* pointer = std::malloc(size == 0 ? 1 : size);
    if (!pointer) throw std::bad_alloc();
    ++live_allocations;
    return pointer;
}
SIRKIT_TEST_NOINLINE void* operator new[](std::size_t size) { return ::operator new(size); }
SIRKIT_TEST_NOINLINE void operator delete(void* pointer) noexcept { if (pointer) { --live_allocations; std::free(pointer); } }
SIRKIT_TEST_NOINLINE void operator delete[](void* pointer) noexcept { ::operator delete(pointer); }
SIRKIT_TEST_NOINLINE void operator delete(void* pointer, std::size_t) noexcept { ::operator delete(pointer); }
SIRKIT_TEST_NOINLINE void operator delete[](void* pointer, std::size_t) noexcept { ::operator delete(pointer); }
#undef SIRKIT_TEST_NOINLINE
static std::uint64_t fingerprint(const WGraph& graph) {
    std::uint64_t result = graph.size();
    for (const auto& edges : graph) {
        result = result * 1000003 + edges.size();
        for (auto edge : edges) result = (result ^ static_cast<std::uint64_t>(edge.to)) * 1000033 + static_cast<std::uint64_t>(edge.w);
    }
    return result;
}
int main() {
    std::size_t cases = 0, failures = 0;
    for (int n : {1, 64, 65, 4096}) for (int kind : {0, 1, 2}) {
        WGraph graph(n);
        if (kind > 0) for (int i = 1; i < n; ++i) graph[i-1].emplace_back(i, -1);
        if (kind == 2) graph[n/2].emplace_back(n/2, -1);
        const auto before = fingerprint(graph);
        for (int fail = 0; fail < 20; ++fail) {
            std::vector<std::int64_t> distances{321, 654};
#ifndef SKIP_RUNTIME_CHECKS
            const auto before_distances = distances;
#endif
            const auto alive_before = live_allocations;
            remaining_allocations = fail;
            fail_allocations = true;
            bool failed = false;
            try { (void)SIRKIT_TEST_NEGATIVE_PATH_API(graph, distances, 0); }
            catch (const std::bad_alloc&) { ++failures; failed = true; }
            fail_allocations = false;
#ifndef SKIP_RUNTIME_CHECKS
            if (failed && distances != before_distances)
                throw std::runtime_error("allocation failure changed checked output");
#else
            (void)failed;
#endif
            if (fingerprint(graph) != before || live_allocations != alive_before)
                throw std::runtime_error("allocation failure changed graph or leaked");
            ++cases;
        }
    }
    std::cout << "{\"cases\":" << cases << ",\"injected_failures\":" << failures << "}\n";
}
