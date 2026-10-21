#include "../DataStructure/dynamic-union-find.cpp"
#include <iostream>

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
    uf.clear();
    std::cout << uf.empty() << '\n'; // 1
}
