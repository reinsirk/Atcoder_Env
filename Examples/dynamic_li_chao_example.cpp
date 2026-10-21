#include "../DataStructure/dynamic-li-chao-tree.cpp"
#include <cstdint>
#include <iostream>

int main() {
    constexpr std::int64_t infinity = std::int64_t{1} << 60;
    DynamicLiChaoTree<std::int64_t, infinity> tree(-10, 10); // 閉区間。
    tree.reserve(32);
    tree.add_line(2, 3);
    tree.add_segment(-1, 0, 0, 4); // 線分も閉区間。
    std::cout << tree.query(2) << ' ' << tree.query(5) << '\n'; // -2 13
    const auto line = tree.query_line(2);
    if (line) std::cout << line->slope << ' ' << line->intercept << '\n'; // -1 0
    const auto nodes = tree.stored_nodes();
    (void)tree.query(0);
    std::cout << (nodes == tree.stored_nodes()) << '\n'; // 1
    std::cout << tree.domain_min() << ' ' << tree.domain_max() << '\n'; // -10 10
    tree.clear();
    std::cout << tree.empty() << ' ' << (tree.query(0) == infinity) << '\n'; // 1 1
}
