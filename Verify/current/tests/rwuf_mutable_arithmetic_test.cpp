// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_RWUF_SOURCE
#define SIRKIT_RWUF_SOURCE "../../../DataStructure/rollback-union-findw.cpp"
#endif
#include SIRKIT_RWUF_SOURCE
#include <cstdio>
#include <cstdlib>

// 左辺が非const参照の加算と、非自明なコピー構築を併せて確認する。
struct MutableAdd {
    std::int64_t value;
    explicit MutableAdd(std::int64_t x = 0) : value(x) {}
    MutableAdd(const MutableAdd& other) : value(other.value) {}
    MutableAdd& operator=(const MutableAdd&) = default;
    friend MutableAdd operator+(MutableAdd& a, const MutableAdd& b) {
        return MutableAdd(a.value + b.value);
    }
    friend MutableAdd operator-(const MutableAdd& a, const MutableAdd& b) {
        return MutableAdd(a.value - b.value);
    }
    friend bool operator==(const MutableAdd&, const MutableAdd&) = default;
};
int main() {
    RollbackWeightedUnionFind<MutableAdd> uf(4);
    if (!uf.unite(0, 1, MutableAdd(3)) || !uf.unite(1, 2, MutableAdd(4))) std::abort();
    if (uf.diff(0, 2).value != 7 || uf.try_diff(2, 0)->value != -7) std::abort();
    if (!uf.consistent(0, 2, MutableAdd(7))) std::abort();
    uf.rollback(1);
    if (uf.same(0, 2) || uf.diff(0, 1).value != 3) std::abort();
    std::puts("PASS mutable-first additive operand compatibility");
}
