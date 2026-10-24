// SPDX-License-Identifier: CC0-1.0
#include "../DataStructure/union_find_w.cpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <utility>
int main() {
    UnionFindW<std::int64_t> uf(5);
    uf.unite(0, 1, 10);
    assert(uf.merge(1, 2, -3) && !uf.merge(0, 2, 7));
    assert(uf.constrain(0, 2, 7) && !uf.constrain(0, 2, 8));
    assert(uf.diff(0, 2) == 7 && uf.try_diff(0, 2) == 7);
    assert(!uf.try_diff(0, 3));
    assert(uf.weight(2) - uf.weight(0) == 7);
    assert(uf.same(0, 2) && uf.issame(0, 1) && uf.root(0) == uf.leader(2));
    assert(uf.size(0) == 3 && uf.size() == 5 && uf.component_count() == 3);
    assert(uf.groups().size() == 3);
    auto copy = uf; UnionFindW<std::int64_t> other; other.swap(copy); assert(other.size() == 5 && copy.size() == 0);
    other.clear(); assert(other.empty());
    other.reset(3);
    assert(other.constrain(0, 2, 9));
    auto reusable = other; reusable.reset();
    assert(reusable.size() == 3 && reusable.component_count() == 3 && reusable.weight(2) == 0);
    swap(uf, other);
    assert(uf.size() == 3 && uf.diff(0, 2) == 9);
    std::cout << uf.size() << ' ' << uf.component_count() << ' ' << other.component_count() << '\n';
}
