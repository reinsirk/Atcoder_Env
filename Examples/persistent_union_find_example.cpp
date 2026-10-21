#include "../DataStructure/persistent-union-find.cpp"
#include <cassert>
int main() {
    PersistentUnionFind uf(5); auto initial = uf.get_root();
    assert(uf.unite(0, 1)); auto branch = uf.get_root();
    uf.merge(1, 2); assert(uf.same(0, 2) && !uf.same(0, 2, branch));
    uf.unite(3, 4, initial); assert(!uf.same(0, 1) && uf.same(3, 4));
    uf.restore(branch); assert(uf.size(0) == 2 && uf.size() == 5);
    assert(uf.find(0) == uf.leader(1) && uf.root_of(0) == uf.find(1));
    assert(uf.find_with_size(0).second == -2);
    assert(uf.component_count() == 4 && uf.groups().size() == 4);
    auto copy = uf; copy.unite(1, 2); assert(!uf.same(0, 2));
    PersistentUnionFind independent(2); assert(independent.size(0) == 1 && uf.size(0) == 2);
    PersistentArray<int> a(std::vector<int>{3, 1, 4}); auto version = a.root;
    a.update(1, 8); assert(a.get(1) == 8 && a.get(version, 1) == 1);
    a.restore(version); assert(a.get(1) == 1 && a.size() == 3);
    a.update(nullptr, 0, 9); assert(a.get(0) == 9 && a.get(1) == 0);
}
