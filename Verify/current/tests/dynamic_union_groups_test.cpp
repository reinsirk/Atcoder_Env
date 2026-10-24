// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_DUF_SOURCE
#define SIRKIT_DUF_SOURCE "../../../DataStructure/dynamic-union-find.cpp"
#endif
#include SIRKIT_DUF_SOURCE
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <new>
#include <source_location>

static int budget = -1;
static std::size_t live = 0, checks = 0, failures = 0;
[[gnu::noinline]] void* operator new(std::size_t n) {
    if (budget == 0) throw std::bad_alloc();
    if (budget > 0) --budget;
    if (void* p = std::malloc(n ? n : 1)) { ++live; return p; }
    throw std::bad_alloc();
}
[[gnu::noinline]] void* operator new[](std::size_t n) { return ::operator new(n); }
[[gnu::noinline]] static void release(void* p) noexcept { if (p) --live; std::free(p); }
void operator delete(void* p) noexcept { release(p); }
void operator delete[](void* p) noexcept { release(p); }
void operator delete(void* p, std::size_t) noexcept { release(p); }
void operator delete[](void* p, std::size_t) noexcept { release(p); }
static void require(bool condition, std::source_location loc = std::source_location::current()) {
    ++checks;
    if (!condition) { std::fprintf(stderr, "failed line %u\n", loc.line()); std::abort(); }
}
static std::vector<int> keys(int n) {
    std::vector<int> result;
    const int edges[] = {std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), -1, 0, 1, -1000000000, 1000000000};
    for (int i = 0; i < n; ++i) result.push_back(i < 7 ? edges[i] : i * 7919 - 900000000);
    return result;
}
static DynamicUnionFind prepared(const std::vector<int>& values, int root, int component_size) {
    DynamicUnionFind uf(values.size());
    for (int key : values) require(uf.add(key));
    const int n = static_cast<int>(values.size());
    const auto index = [=](int i) { return (root + n - i) % n; };
    for (int stride = 1; stride < component_size; stride *= 2)
        for (int i = 0; i + stride < n; i += stride * 2)
            require(uf.unite(values[index(i)], values[index(i + stride)]));
    return uf;
}
static void run() {
    for (int n : {0, 1, 2, 7, 31, 32, 33, 64, 129}) {
        const auto values = keys(n);
        for (int root : {0, n / 2, n ? n - 1 : 0}) for (int state : {0, 1, 2}) {
            const int component_size = state == 0 ? 1 : state == 1 ? n : 2;
            auto initial = prepared(values, root, component_size);
            std::vector<int> parents;
            for (int key : values) parents.push_back(initial.data(key));
            std::vector<std::vector<int>> expected;
            if (state == 0) { for (int key : values) expected.push_back({key}); }
            else if (state == 1 && n) expected.push_back(values);
            else {
                // 一般成分の期待値は、公開findの代表で独立にキーを集める。
                auto copy = initial;
                std::vector<int> leaders;
                for (int key : values) leaders.push_back(copy.find(key));
                for (int i = 0; i < n; ++i) if (leaders[i] == values[i]) {
                    expected.emplace_back();
                    for (int j = 0; j < n; ++j) if (leaders[j] == values[i]) expected.back().push_back(values[j]);
                }
            }
            bool success = false;
            for (int allowance = 0; allowance < n + 10; ++allowance) {
                auto uf = initial;
                const auto owned = live;
                budget = allowance;
                bool caught = false;
                try {
                    const auto result = uf.groups();
                    require(result == expected);
                } catch (const std::bad_alloc&) { caught = true; ++failures; }
                budget = -1;
                require(live == owned && uf.node_count() == values.size() && uf.component_count() == expected.size());
                if (caught) {
                    for (int i = 0; i < n; ++i) require(uf.data(values[i]) == parents[i]);
                } else {
                    if (state == 1 && n) for (int i = 0; i < n; ++i)
                        require(uf.data(values[i]) == (i == root ? -n : values[root]));
                    for (int key : values) {
                        require(uf.contains(key));
                        require(uf.find(key) == initial.find(key) && uf.size(key) == initial.size(key));
                    }
                    success = true;
                    break;
                }
            }
            require(success);
        }
    }
}
int main() {
    const auto initial = live;
    run(); require(live == initial);
    std::printf("PASS groups checks=%zu allocation_failures=%zu live_delta=%td\n", checks, failures, static_cast<std::ptrdiff_t>(live) - static_cast<std::ptrdiff_t>(initial));
}
