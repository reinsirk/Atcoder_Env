#include "../DataStructure/erasable-priority-queue.cpp"
#include <iostream>
#include <vector>

int main() {
    ErasablePriorityQueue<int> queue(std::vector<int>{5, 2, 2, 8}); // 最小値優先。
    queue.erase(2); // 存在する値を1個消す。
    std::cout << queue.size() << ' ' << queue.top() << '\n'; // 3 2
    queue.emplace(1);
    if (auto value = queue.extract_top()) std::cout << *value << '\n'; // 1
    queue.pop();
    if (auto value = queue.try_top()) std::cout << *value << '\n'; // 5
    queue.clear();
    std::cout << queue.empty() << ' ' << queue.try_top().has_value() << '\n'; // 1 0
    ErasablePriorityQueue<int, false> maximum;
    maximum.push(3);
    maximum.push(7);
    std::cout << maximum.top() << '\n'; // 7
}
