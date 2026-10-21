// SPDX-License-Identifier: CC0-1.0
#include <cstdint>
#include <iostream>
#include <string>

int main() {
    std::ios::sync_with_stdio(false); std::cin.tie(nullptr);
    Heap heap;
    std::string command;
    while (std::cin >> command && command != "end") {
        if (command == "insert") { int value; std::cin >> value; heap.push(-value); }
        else std::cout << -heap.pop_value() << '\n';
    }
}
