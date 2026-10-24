// SPDX-License-Identifier: CC0-1.0
// 内部表を4本体と重複して含め、整数・汎用値の共有利用を検査する。
#ifndef SIRKIT_INTERNAL_HASH_SOURCE
#define SIRKIT_INTERNAL_HASH_SOURCE "../../../Internal/Nyaan_hash_map_min.cpp"
#endif
#include SIRKIT_INTERNAL_HASH_SOURCE
#include "../../../DataStructure/dynamic-binary-indexed-tree.cpp"
#include "../../../DataStructure/dynamic-binary-indexed-tree-2d.cpp"
#include "../../../DataStructure/dynamic-union-find.cpp"
#include SIRKIT_INTERNAL_HASH_SOURCE
#include "../../../DataStructure/dynamic-union-find.cpp"
#include "../../../DataStructure/dynamic-binary-indexed-tree-2d.cpp"
#include "../../../DataStructure/dynamic-binary-indexed-tree.cpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
int main() {
    reinsirk::detail::InternalHashMap<std::int64_t, std::string> table;
    table[std::numeric_limits<std::int64_t>::min()] = "min";
    table[std::numeric_limits<std::int64_t>::max()] = "max";
    assert(table.size() == 2 && *table.find(std::numeric_limits<std::int64_t>::min()) == "min");
    DynamicUnionFind uf;
    uf.unite(std::numeric_limits<int>::min(), -1);
    uf.unite(0, std::numeric_limits<int>::max());
    assert(uf.node_count() == 4 && !uf.same(-1,0));
    uf.unite(-1,0); assert(uf.size(0) == 4);
    auto copy = uf; uf.clear(); assert(copy.size(-1) == 4 && uf.empty());
    DynamicFenwickTree<int, double> one(1000000000);
    one.add(999999999, 1.25); one.add(0, 2.5);
    assert(one.sum_all() == 3.75 && one.get(0) == 2.5);
    auto one_copy = one; one.clear(); assert(one.sum_all() == 0 && one_copy.sum_all() == 3.75);
    DynamicFenwickTree2D<double> two(1000000000,1000000000);
    two.add(1,2,1.25); two.add(999999999,999999999,2.5);
    assert(two.sum_all() == 3.75 && two.get(1,2) == 1.25);
    auto two_copy = two; two.clear(); assert(two.sum_all() == 0 && two_copy.sum_all() == 3.75);
    std::cout << "PASS shared guard, extreme-key UF, floating 1D/2D Fenwick with internal hash\n";
}
