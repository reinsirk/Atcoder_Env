// SPDX-License-Identifier: CC0-1.0

#include <cassert>
#include <iostream>

#include "../DataStructure/rollback-union-find.cpp"
int main() {
    RollbackUnionFind uf(5); uf.reserve_history(20);
    uf.unite(0, 1); uf.snapshot(); const auto state = uf.get_state();
    assert(uf.merge(1, 2)); assert(!uf.unite(0, 2));
    uf.undo(); assert(uf.same(0, 2));
    uf.rollback(); assert(!uf.issame(0, 2) && uf.size(0) == 2);
    uf.unite(3, 4); uf.rollback(state);
    assert(!uf.same(3, 4) && uf.find(0) == uf.root(1) && uf.leader(0) == uf.find(1));
    assert(uf.size() == 5 && uf.component_count() == 4 && uf.groups().size() == 4);
    auto copy = uf; RollbackUnionFind other; other.swap(copy); assert(other.size() == 5 && copy.size() == 0);

    // 今の連結状態を新しい基準にし、過去の履歴を消す。容量は再利用する。
    other.clear_history();
    assert(other.get_state() == 0 && other.same(0, 1));
    other.rollback(); assert(other.component_count() == 4);
    other.unite(1, 2); other.undo();
    assert(other.size(0) == 2 && !other.same(0, 2));
    assert(uf.get_state() == state); // コピー元の履歴は変わらない。
    auto reused = other;
    reused = uf; // 確保済みの頂点・履歴領域をコピー代入に再利用する。
    reused.reset(); // 頂点数を保って全成分と履歴を初期化する。
    assert(reused.size() == 5 && reused.component_count() == 5 && reused.get_state() == 0);
    reused.clear(); assert(reused.empty());
    reused.reset(2); swap(reused, copy);
    assert(copy.size() == 2 && reused.empty());
    std::cout << other.component_count() << ' ' << other.get_state() << '\n';
}
