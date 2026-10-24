// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_PUF_SOURCE
#define SIRKIT_PUF_SOURCE "../../../DataStructure/persistent-union-find.cpp"
#endif
#include SIRKIT_PUF_SOURCE
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

static const void* volatile escaped;
static std::uint64_t next(std::uint64_t& s) { s ^= s << 13; s ^= s >> 7; return s ^= s << 17; }
int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "same_large";
    std::uint64_t checksum = 0;
    double milliseconds = 0;
    auto measure = [&](auto&& operation) {
        const auto start = std::chrono::steady_clock::now();
        operation();
        milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    };
    if (mode == "same_small" || mode == "same_large") {
        const int n = mode == "same_small" ? 32 : 32768;
        PersistentUnionFind uf(n);
        std::vector<PersistentUnionFind::Node*> roots;
        for (int stride = 1; stride < n; stride *= 2) for (int i = 0; i + stride < n; i += stride * 2) {
            uf.unite(i, i + stride); if ((i & 255) == 0) roots.push_back(uf.root);
        }
        struct Query { int a, b; PersistentUnionFind::Node* root; };
        std::vector<Query> queries; queries.reserve(2000000);
        std::uint64_t state = 41;
        for (int i = 0; i < 2000000; ++i) queries.push_back({int(next(state) % n), int(next(state) % n), roots[next(state) % roots.size()]});
        measure([&] { for (const auto& q : queries) checksum += uf.same(q.a, q.b, q.root); });
    } else if (mode == "vector_int_small" || mode == "vector_int_large") {
        const int n = mode == "vector_int_small" ? 32 : 200000;
        const int repeats = n == 32 ? 1000000 : 500;
        std::vector<int> values(n, 7);
        measure([&] { for (int r = 0; r < repeats; ++r) { PersistentArray<int> array(values); escaped = array.root; checksum += array.size(); } });
    } else if (mode == "vector_string_small" || mode == "vector_string_large") {
        const int n = mode == "vector_string_small" ? 32 : 5000;
        const int repeats = n == 32 ? 300000 : 2000;
        std::vector<std::string> values(n, "abcdef");
        measure([&] { for (int r = 0; r < repeats; ++r) { PersistentArray<std::string> array(values); escaped = array.root; checksum += array.size(); } });
    } else if (mode == "constructor") {
        measure([&] { for (int r = 0; r < 1000000; ++r) { PersistentUnionFind uf(32); escaped = uf.root; checksum += uf.size(r % 32); } });
    } else return 2;
    std::printf("%.6f %ju\n", milliseconds, static_cast<std::uintmax_t>(checksum));
}
