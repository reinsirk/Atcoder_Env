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
    std::ios::sync_with_stdio(false); std::cin.tie(nullptr);
    int n; std::cin >> n;
    std::vector<std::int64_t> values(n);
    for (auto& x : values) std::cin >> x;
    RadixSort(values);
    for (int i = 0; i < n; ++i) std::cout << values[i] << (i + 1 == n ? '\n' : ' ');
}
