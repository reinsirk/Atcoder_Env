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
    IntegerSieve sieve(10000);
    std::mt19937 generator(81234);
    for (int repetition = 0; repetition < 120; ++repetition) {
        const int maximum = repetition < 60 ? 1 + repetition : 9000 + static_cast<int>(generator() % 1000);
        auto keys = sieve.sorted_divisors(maximum);
        std::vector<std::pair<int, std::int64_t>> original;
        for (int key : keys) original.push_back({key, static_cast<int>(generator() % 101) - 50});
        auto expected = original;
        for (auto& [key, value] : expected) {
            value = 0;
            for (auto [other, weight] : original) if (other % key == 0) value += weight;
        }
        auto actual = original;
        std::shuffle(actual.begin(), actual.end(), generator);
        sparse_multiple_zeta_transform(actual, sieve);
        assert(actual == expected);
        sparse_multiple_mobius_transform(actual, sieve);
        assert(actual == original);
    }
    std::vector<std::pair<int, int>> empty;
    sparse_multiple_zeta_transform(empty, sieve); sparse_multiple_mobius_transform(empty, sieve);
    std::cout << "OK\n";
}
