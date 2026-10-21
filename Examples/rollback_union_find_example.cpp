#include "../DataStructure/rollback-union-find.cpp"
#include <cassert>
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
}
