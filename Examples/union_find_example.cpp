// SPDX-License-Identifier: CC0-1.0
#include "../DataStructure/union_find.cpp"
#include <cassert>
#include <iostream>
#include <utility>
int main() {
    UnionFind uf(5);
    uf.unite(0, 1);
    assert(uf.merge(1, 2) && !uf.merge(0, 2));
    assert(uf.same(0, 2) && uf.issame(0, 1));
    assert(uf.root(0) == uf.leader(2) && uf.size(1) == 3);
    assert(uf.size() == 5 && uf.component_count() == 3);
    auto groups = uf.groups(); assert(groups.size() == 3);
    auto copy = uf; copy.merge(3, 4); assert(!uf.same(3, 4));
    auto moved = std::move(copy); assert(copy.size() == 0 && moved.same(3, 4));
    moved.clear(); assert(moved.empty() && moved.component_count() == 0);
    moved.reset(3); assert(moved.component_count() == 3);
    moved.unite(0, 2);
    auto reusable = moved; reusable.reset();
    assert(reusable.size() == 3 && reusable.component_count() == 3 && !reusable.same(0, 2));
    uf.swap(moved); assert(uf.size() == 3 && uf.same(0, 2));
    swap(uf, moved);
    std::cout << moved.size() << ' ' << moved.component_count()
              << ' ' << uf.component_count() << '\n'; // 3 2 3
}
