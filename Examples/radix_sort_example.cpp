// SPDX-License-Identifier: CC0-1.0
#include "../Algorithm/Radix_sort.cpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    std::vector<std::int64_t> values{7, -4, 7, 0, -9};
    const auto order = RadixSortOrder(values);
    for (auto index : order) std::cout << index << ' ';
    std::cout << '\n'; // 4 1 3 0 2（同値は入力順）
    RadixSort(values);
    for (auto x : values) std::cout << x << ' ';
    std::cout << '\n'; // -9 -4 0 7 7
    RadixSortWorkspace<std::int64_t> workspace;
    workspace.reserve(100);
    values = {3, -1, 2};
    workspace.sort(values, std::int64_t{3}); // 全要素が指定した上限以下であること。
    std::cout << values.front() << ' ' << values.back() << '\n'; // -1 3
    std::vector<std::int64_t> many(257);
    for (std::size_t i = 0; i < many.size(); ++i)
        many[i] = static_cast<std::int64_t>(i * 137 % 257) - 128;
    workspace.sort(many); // 作業領域を別の配列にも再利用する。
    assert(std::is_sorted(many.begin(), many.end()));
    workspace.release();
    std::cout << workspace.capacity() << '\n'; // 0
}
