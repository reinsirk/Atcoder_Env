#include "../DataStructure/heap.cpp"
#include <iostream>
#include <vector>

int main() {
    Heap heap(std::vector<int>{4, 1, 3}); // O(N)で最小ヒープを作る。
    heap.push(2);
    std::cout << heap.top() << ' ' << heap.size() << '\n'; // 1 4
    std::cout << heap.pop_value() << '\n'; // 1
    std::cout << heap.replace_top(5) << ' ' << heap.top() << '\n'; // 2 3
    std::cout << heap.push_pop(0) << ' ' << heap.top() << '\n'; // 0 3
    Heap copy = heap; // 独立したコピー。
    while (!copy.empty()) std::cout << copy.pop_value() << ' '; // 3 4 5
    std::cout << '\n';
    heap.clear();
    heap.pop(); // 空なら何もしない。
    std::cout << heap.top() << ' ' << heap.top_or(99) << '\n'; // -1 99
}
