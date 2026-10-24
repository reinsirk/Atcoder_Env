// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_RWUF_SOURCE
#define SIRKIT_RWUF_SOURCE "../../../DataStructure/rollback-union-findw.cpp"
#endif
#include SIRKIT_RWUF_SOURCE
#include <atcoder/modint>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
struct Ring {
    std::uint64_t x;
    explicit Ring(std::int64_t value = 0) : x(static_cast<std::uint64_t>(value)) {}
    friend Ring operator+(Ring a, Ring b) { a.x += b.x; return a; }
    friend Ring operator-(Ring a, Ring b) { a.x -= b.x; return a; }
    friend bool operator==(Ring, Ring) = default;
};
#if SIRKIT_VALUE_TYPE == 0
using Value = atcoder::modint998244353;
static std::uint64_t value_of(Value value) { return value.val(); }
#elif SIRKIT_VALUE_TYPE == 1
using Value = std::int64_t;
static std::uint64_t value_of(Value value) { return static_cast<std::uint64_t>(value); }
#else
using Value = Ring;
static std::uint64_t value_of(Value value) { return value.x; }
#endif
int main(int argc, char** argv) {
    if (argc != 3) return 2;
    const int n = std::atoi(argv[1]), repeats = std::atoi(argv[2]);
    if (n < 3 || repeats < 1) return 2;
    RollbackWeightedUnionFind<Value> uf(n);
    std::uint64_t sum = 0;
    const auto start = std::chrono::steady_clock::now();
    for (int r = 0; r < repeats; ++r) {
        const int count = n - (r & 1);
        uf.reset(count);
        const Value gap(r % 17);
        if (!uf.unite(count - 2, count - 1, gap)) return 3;
        const auto difference = uf.diff(count - 2, count - 1);
        if (!(difference == gap)) return 3;
        sum += value_of(difference) + uf.get_state() + uf.component_count();
    }
    const auto stop = std::chrono::steady_clock::now();
    std::printf("%.6f %ju\n", std::chrono::duration<double, std::milli>(stop - start).count(), static_cast<std::uintmax_t>(sum));
}
