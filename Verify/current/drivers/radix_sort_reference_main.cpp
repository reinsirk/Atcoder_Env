// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <limits>
#include <random>
#include <string>
#include <utility>
#include <vector>

template<class T> void verify_radix(std::mt19937_64& generator) {
    RadixSortWorkspace<T> workspace;
    for (int n : {0, 1, 2, 63, 64, 65, 1000}) {
        std::vector<T> values(n);
        for (auto& x : values) x = static_cast<T>(generator());
        if (n >= 2) { values[0] = std::numeric_limits<T>::min(); values[1] = std::numeric_limits<T>::max(); }
        auto expected = values; std::stable_sort(expected.begin(), expected.end());
        auto actual = values; workspace.sort(actual); assert(actual == expected);
        actual = values; RadixSort(actual); assert(actual == expected);
        actual = values; workspace.sort(actual, std::numeric_limits<T>::max()); assert(actual == expected);
        auto order = RadixSortOrder(values);
        std::vector<std::size_t> reference(values.size()); std::iota(reference.begin(), reference.end(), 0);
        std::stable_sort(reference.begin(), reference.end(), [&](auto a, auto b) { return values[a] < values[b]; });
        assert(order == reference);
        std::reverse(expected.begin(), expected.end()); workspace.sort(expected); assert(expected == actual);
    }
    workspace.release(); assert(workspace.capacity() == 0);
}
int main() {
    std::mt19937_64 generator(91234);
    verify_radix<std::int8_t>(generator); verify_radix<std::uint8_t>(generator);
    verify_radix<std::int16_t>(generator); verify_radix<std::uint16_t>(generator);
    verify_radix<std::int32_t>(generator); verify_radix<std::uint32_t>(generator);
    verify_radix<std::int64_t>(generator); verify_radix<std::uint64_t>(generator);
    std::cout << "OK\n";
}
