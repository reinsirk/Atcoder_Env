// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::int64_t n; std::cin >> n;
    int bound = 1;
    while (std::int64_t(bound) * bound < n) ++bound;
    IntegerSieve sieve(bound);
    std::cout << n << ':';
    for (auto [prime, exponent] : sieve.prime_factorization(n))
        for (int i = 0; i < exponent; ++i) std::cout << ' ' << prime;
    std::cout << '\n';
}
