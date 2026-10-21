// SPDX-License-Identifier: CC0-1.0
#include "../DataStructure/rollback-union-findw.cpp"
#include <cassert>
#include <cstdint>

int main() {
    using UnionFind = RollbackWeightedUnionFind<std::int64_t>;
    UnionFind uf(4);
    uf.reserve_history(10);
    assert(uf.unite(0, 1, 3)); // potential(1) - potential(0) = 3
    uf.snapshot();
    assert(uf.unite(1, 2, -5));
    assert(uf.diff(0, 2) == -2);
    assert(uf.unite_checked(0, 2, -2) == UnionFind::MergeResult::AlreadyConsistent);
    assert(uf.unite_checked(0, 2, 10) == UnionFind::MergeResult::Contradiction);
    assert(uf.get_state() == 4); // 冗長・矛盾の追加も各1回として数える。
    uf.undo();
    uf.rollback();
    assert(uf.same(0, 1) && !uf.same(0, 2));
    assert(!uf.try_diff(0, 2));
    assert(uf.consistent(0, 2, 7)); // 未連結なので新しい条件を追加できる。
    assert(uf.size(0) == 2 && uf.component_count() == 3);
    auto copy = uf;
    assert(copy.unite(2, 3, 7));
    assert(!uf.same(2, 3));
    assert(copy.groups().size() == 2);
    uf.reset(2);
    assert(uf.size() == 2 && uf.get_state() == 0);
}
