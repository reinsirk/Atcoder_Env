// SPDX-License-Identifier: CC0-1.0
#include "../Graph/LCA.cpp"
#include <iostream>
#include <vector>
#include <utility>

int main() {
    std::vector<std::vector<int>> forest{{1, 2}, {0, 3, 4}, {0}, {1}, {1}, {6}, {5}};
    LCA tree(forest);
    std::cout << tree.get(3, 4) << ' ' << tree.get_distance(3, 2) << '\n'; // 1 3
    std::cout << tree.kth_ancestor(3, 2) << ' ' << tree.kth_on_path(3, 2, 2) << '\n'; // 0 0
    std::cout << tree.get(3, 2, 4) << ' ' << tree.is_ancestor(1, 4) << '\n'; // 1 1
    std::cout << tree.get(0, 5) << ' ' << tree.component_count() << '\n'; // -1 2
    std::cout << tree.parent(3) << ' ' << tree.depth_of(3) << ' ' << tree.root_of(6) << '\n'; // 1 2 5
    const auto [first, last] = tree.subtree_range(1);
    std::cout << tree.subtree_size(1) << ' ' << (last - first) << '\n'; // 3 3
    // 頂点ごとの値をこの添字へ並べると、部分木が半開区間になる。
    const int index = tree.order_index(4);
    std::cout << (first <= index && index < last) << '\n'; // 1
    LCA copy = tree;
    LCA moved = std::move(copy);
    std::cout << copy.empty() << ' ' << moved.get_distance(3, 2) << '\n'; // 1 3
    tree.build(std::vector<std::vector<int>>{{}});
    std::cout << tree.get(0, 0) << '\n'; // 0
    tree.build(std::vector<std::vector<int>>{{1}, {0}, {}});
    std::cout << tree.parent(1) << ' ' << tree.get(0, 2) << '\n'; // 0 -1
    tree.clear();
    std::cout << tree.empty() << '\n'; // 1
}
