#include "../DataStructure/union_find.cpp"
#include <cassert>
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
}
