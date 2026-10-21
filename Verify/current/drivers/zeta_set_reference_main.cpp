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
    std::mt19937 generator(71234);
    for (int exponent = 0; exponent <= 8; ++exponent) {
        const int n = 1 << exponent;
        std::vector<std::int64_t> a(n);
        for (auto& x : a) x = static_cast<int>(generator() % 101) - 50;
        for (bool upper : {false, true}) {
            auto expected = a;
            for (int i = 0; i < n; ++i) {
                expected[i] = 0;
                for (int j = 0; j < n; ++j)
                    if (upper ? (i & j) == i : (i & j) == j) expected[i] += a[j];
            }
            auto actual = a;
            if (upper) superset_zeta_transform(actual); else subset_zeta_transform(actual);
            assert(actual == expected);
            if (upper) superset_mobius_transform(actual); else subset_mobius_transform(actual);
            assert(actual == a);
        }
    }
    std::vector<int> empty;
    subset_zeta_transform(empty); subset_mobius_transform(empty);
    superset_zeta_transform(empty); superset_mobius_transform(empty);
    std::cout << "OK\n";
}
