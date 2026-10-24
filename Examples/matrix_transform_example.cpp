// SPDX-License-Identifier: CC0-1.0
#include "../Algorithm/rotate.cpp"
// SIMDを除く場合は、include前にSIRKIT_DISABLE_SIMDを定義する。
#include <cassert>
#include <limits>
#include <string>
#include <vector>

int main() {
    const std::vector<std::vector<int>> a{{1, 2, 3}, {4, 5, 6}};
    assert((rotate_90r(a) == std::vector<std::vector<int>>{{4, 1}, {5, 2}, {6, 3}}));
    assert((rotate_90l(a) == std::vector<std::vector<int>>{{3, 6}, {2, 5}, {1, 4}}));
    assert((rotate_180(a) == std::vector<std::vector<int>>{{6, 5, 4}, {3, 2, 1}}));
    assert((transpose(a) == std::vector<std::vector<int>>{{1, 4}, {2, 5}, {3, 6}}));
    assert(rotate_quarter(a, -1) == rotate_90l(a));
    assert(rotate_quarter(a, std::numeric_limits<int>::min()) == a);
    auto b = a;
    flip_i_inplace(b);
    assert(b == flip_i(a));
    flip_j_inplace(b);
    assert(b == rotate_180(a));
    const std::vector<std::string> board{"abc", "def"};
    assert((rotate_90r(board) == std::vector<std::string>{"da", "eb", "fc"}));
    const std::vector<std::vector<bool>> bits{{true, false}, {false, true}};
    assert(rotate_180(bits) == bits);
    const std::vector<std::vector<int>> empty;
    assert(rotate_90r(empty).size() == 1); // 既存3関数の互換仕様。
    assert(transpose(empty).empty());
}
