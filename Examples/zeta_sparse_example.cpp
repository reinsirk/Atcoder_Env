// SPDX-License-Identifier: CC0-1.0
#include "../Math/Zeta/Sparse_Transform.cpp"
#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

int main() {
    IntegerSieve sieve(100);
    const std::vector<std::pair<int, int>> original{{1, 1}, {2, 2}, {3, 3}, {6, 6}};
    auto values = original;
    sparse_multiple_zeta_transform(values, sieve);
    assert(values.front().second == 12);

    // 1..m順なら、並べ替えと補助配列の確保を省く。
    const std::vector<std::pair<int, int>> dense_original{{1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}, {6, 6}};
    auto dense = dense_original;
    sparse_multiple_zeta_transform(dense, sieve);
    std::cout << values.front().second << ' ' << dense.front().second << '\n'; // 12 21
    sparse_multiple_mobius_transform(dense, sieve);
    assert(dense == dense_original);

    sparse_multiple_mobius_transform(values, sieve);
    assert(values == original);
}
