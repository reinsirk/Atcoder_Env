// SPDX-License-Identifier: CC0-1.0
#include "../Algorithm/next_combination.cpp"
#include <cstdint>
#include <iostream>
int main() {
    std::cout << next_combination(7) << '\n'; // 11
    std::cout << last_combination<std::uint64_t>(5, 3) << '\n'; // 28
    auto mask = first_combination<std::uint64_t>(3);
    do { std::cout << mask << ' '; } while (next_combination(mask, 5));
    std::cout << '\n'; // 7 11 13 14 19 21 22 25 26 28
    for_each_combination(3, 0, [](auto value) { std::cout << value << '\n'; }); // 0を1回
    for_each_combination(64, 64, [](auto value) { std::cout << value << '\n'; }); // 全64ビットが1
}
