#include "../DataStructure/dynamic-binary-indexed-tree.cpp"
#include <cstdint>
#include <iostream>

int main() {
    DynamicFenwickTree<std::int64_t, std::int64_t> bit(1000000000);
    bit.reserve(100);
    bit.add(2, 3);
    bit.set(10, 5);
    std::cout << bit.get(10) << ' ' << bit.sum(0, 11) << '\n'; // 5 8
    // 検索は全要素が非負のときに使う。
    std::cout << bit.lower_bound(3) << ' ' << bit.upper_bound(3) << '\n'; // 2 10
    std::cout << bit.size() << ' ' << bit.sum_all() << '\n'; // 1000000000 8
    const auto nodes = bit.stored_nodes();
    (void)bit.sum(100);
    std::cout << (nodes == bit.stored_nodes()) << '\n'; // 1（読み取りでは増えない）
    bit.clear();
    std::cout << bit.empty() << ' ' << bit.sum_all() << '\n'; // 0 0（長さは維持）
    bit.reset(0);
    std::cout << bit.empty() << '\n'; // 1
}
