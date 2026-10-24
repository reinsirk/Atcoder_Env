// SPDX-License-Identifier: CC0-1.0
#include "../Math/Zeta/Gcd_Lcm_Convolution.cpp"
#include <cassert>
#include <vector>
#include <iostream>

int main() {
    const std::vector<int> f{2, 3, 5}, g{7, 11, 13};
    IntegerSieve sieve(2);
    assert((gcd_convolution(f, g) == std::vector<int>{14, 170, 126}));
    assert((lcm_convolution(f, g) == std::vector<int>{118, 33, 159}));
    assert(gcd_convolution(f, g, sieve) == gcd_convolution(f, g));
    assert(lcm_convolution(f, g, sieve) == lcm_convolution(f, g));
    assert((gcd_square(f, sieve) == std::vector<int>{4, 51, 45}));
    assert((lcm_square(f) == std::vector<int>{36, 9, 55}));
    assert(gcd_square(f) == gcd_convolution(f, f));
    assert(lcm_square(f, sieve) == lcm_convolution(f, f, sieve));
    for (auto x : gcd_square(f, sieve)) std::cout << x << ' ';
    std::cout << '\n'; // 4 51 45
}
