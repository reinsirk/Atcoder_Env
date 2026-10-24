#include "../DataStructure/dynamic-union-find.cpp"
#include <iostream>
#include <cassert>

int main() {
    DynamicUnionFind uf(10);
    uf.add(-7);
    uf.unite(-7, 1000000000);
    uf.unite(3, 4, [](int root, int absorbed) {
        std::cout << root << ' ' << absorbed << '\n'; // 3 4
    });
    std::cout << uf.same(-7, 1000000000) << ' ' << uf.size(-7) << '\n'; // 1 2
    std::cout << uf.contains(99) << ' ' << uf.node_count() << '\n'; // 0 4（containsは登録しない）
    std::cout << uf.component_count() << ' ' << uf.groups().size() << '\n'; // 2 2
    (void)uf.find(99); // 未知のキーは自動登録。
    std::cout << uf.node_count() << ' ' << uf.component_count() << '\n'; // 5 3
    DynamicUnionFind connected;
    connected.add(-7); connected.add(1000000000);
    assert(connected.groups().size() == 2); // 全て独立な場合。
    connected.unite(1000000000, -7); // 代表は先に登録したキーとは限らない。
    const auto all = connected.groups();
    assert(all.size() == 1 && all[0][0] == -7 && all[0][1] == 1000000000);
    assert(connected.data(-7) == 1000000000 && connected.size(-7) == 2);
    uf.clear();
    std::cout << uf.empty() << '\n'; // 1
}
