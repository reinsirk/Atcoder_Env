// SPDX-License-Identifier: CC0-1.0
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
#ifndef SIRKIT_TEST_TOPOLOGICAL_SOURCE
#define SIRKIT_TEST_TOPOLOGICAL_SOURCE "../../../Graph/Topological.cpp"
#endif
#include SIRKIT_TEST_TOPOLOGICAL_SOURCE
static bool fail_allocations = false;
static int remaining_allocations = 0;
static std::size_t live_allocations = 0;
void* operator new(std::size_t size) {
    if (fail_allocations && remaining_allocations-- == 0) throw std::bad_alloc();
    void* pointer = std::malloc(size == 0 ? 1 : size);
    if (!pointer) throw std::bad_alloc();
    ++live_allocations;
    return pointer;
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* pointer) noexcept { if (pointer) { --live_allocations; std::free(pointer); } }
void operator delete[](void* pointer) noexcept { ::operator delete(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { ::operator delete(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { ::operator delete(pointer); }
int main() {
    std::size_t cases = 0, failures = 0;
    for (int n : {0, 1, 64, 65, 4097}) {
        Graph graph(n);
        if (n > 1) graph[0].push_back(n - 1);
        const Graph before = graph;
        for (int api = 0; api < 5; ++api) for (int fail = 0; fail < 8; ++fail) {
            const auto alive_before = live_allocations;
            remaining_allocations = fail;
            fail_allocations = true;
            try {
                if (api == 0) (void)TopologicalSort(graph);
                if (api == 1) (void)TopologicalSortLexicographical(graph);
                if (api == 2) (void)TopologicalSortDetails(graph);
                if (api == 3) (void)TopologicalSortDetails(graph, true);
                if (api == 4) (void)IsDAG(graph);
            } catch (const std::bad_alloc&) { ++failures; }
            fail_allocations = false;
            if (graph != before || live_allocations != alive_before)
                throw std::runtime_error("topological allocation failure changed input or leaked");
            ++cases;
        }
    }
    std::cout << "{\"cases\":" << cases << ",\"injected_failures\":" << failures << "}\n";
}
