// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_RWUF_SOURCE
#define SIRKIT_RWUF_SOURCE "../../../DataStructure/rollback-union-findw.cpp"
#endif
#include SIRKIT_RWUF_SOURCE
#include <cstdint>
#include <cstdio>
#include <cstdlib>

static std::size_t checks = 0;
static void require(bool condition) { ++checks; if (!condition) std::abort(); }
struct NoDefault {
    std::int64_t x;
    NoDefault() = delete;
    explicit NoDefault(std::int64_t value) noexcept : x(value) {}
    friend NoDefault operator+(NoDefault a, NoDefault b) { return NoDefault(a.x + b.x); }
    friend NoDefault operator-(NoDefault a, NoDefault b) { return NoDefault(a.x - b.x); }
    friend bool operator==(NoDefault, NoDefault) = default;
};
struct DifferentDefault {
    static inline int default_calls = 0;
    std::int64_t x;
    DifferentDefault() noexcept : x(111) { ++default_calls; }
    explicit DifferentDefault(std::int64_t value) noexcept : x(value) {}
    friend DifferentDefault operator+(DifferentDefault a, DifferentDefault b) { return DifferentDefault(a.x + b.x); }
    friend DifferentDefault operator-(DifferentDefault a, DifferentDefault b) { return DifferentDefault(a.x - b.x); }
    friend bool operator==(DifferentDefault, DifferentDefault) = default;
};
template<class T> static void exercise() {
    RollbackWeightedUnionFind<T> uf;
    for (int n : {0, 32, 8, 65, 2, 0, 1}) {
        uf.reset(n);
        require(uf.size() == n && uf.component_count() == n && uf.get_state() == 0);
        for (int v = 0; v < n; ++v) require(uf.get_weight(v).x == 0 && uf.size(v) == 1);
        for (int v = 1; v < n; ++v) require(uf.unite(v - 1, v, T(3)));
        if (n) require(uf.diff(0, n - 1).x == 3 * (n - 1));
        uf.snapshot(); uf.reset();
        require(uf.size() == n && uf.component_count() == n && uf.get_state() == 0);
        for (int v = 0; v < n; ++v) require(uf.get_weight(v).x == 0 && uf.find(v) == v);
        uf.rollback(); require(uf.get_state() == 0);
    }
}
int main() {
    exercise<NoDefault>(); exercise<DifferentDefault>();
    require(DifferentDefault::default_calls == 0);
    std::printf("PASS reset explicit zero checks=%zu default_calls=%d\n", checks, DifferentDefault::default_calls);
}
