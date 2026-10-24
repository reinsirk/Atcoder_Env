// SPDX-License-Identifier: CC0-1.0
#include "../Math/Zeta/Bitwise_Convolution.cpp"
#include <cassert>
#include <vector>
#include <iostream>

int main() {
    const std::vector<int> f{1, 2}, g{3, 4};
    assert((bitwise_and_convolution(f, g) == std::vector<int>{13, 8}));
    assert((bitwise_or_convolution(f, g) == std::vector<int>{3, 18}));
    const auto and_square = bitwise_and_square(f);
    const auto or_square = bitwise_or_square(f);
    assert((and_square == std::vector<int>{5, 4}));
    assert((or_square == std::vector<int>{1, 8}));
    std::cout << and_square[0] << ' ' << and_square[1] << '\n'; // 5 4
    std::cout << or_square[0] << ' ' << or_square[1] << '\n'; // 1 8
}
