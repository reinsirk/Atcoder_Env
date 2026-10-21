#include "../DataStructure/dynamic-binary-indexed-tree-2d.cpp"
#include <cstdint>
#include <iostream>

int main() {
    DynamicFenwickTree2D<std::int64_t> bit(1000000000, 1000000000);
    bit.reserve(2000); // 内部セル数の目安による予約ヒント。各行の内容量は必要時に確保。
    bit.add(2, 3, 4);
    bit.set(5, 7, 9);
    std::cout << bit.get(5, 7) << ' ' << bit.sum(0, 0, 6, 8) << '\n'; // 9 13
    std::cout << bit.sum(6, 8) << ' ' << bit.sum_all() << '\n'; // 13 13
    const auto nodes = bit.stored_nodes();
    (void)bit.get(0, 0);
    std::cout << (nodes == bit.stored_nodes()) << '\n'; // 1
    bit.clear();
    std::cout << bit.rows() << ' ' << bit.cols() << ' ' << bit.sum_all() << '\n'; // 1000000000 1000000000 0
    bit.reset(0, 0);
    std::cout << bit.empty() << '\n'; // 1
}
