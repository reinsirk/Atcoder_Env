// SPDX-License-Identifier: CC0-1.0
#include "../DataStructure/Compressed_Binary_Trie.cpp"
#include <iostream>
#include <cstdint>

int main()
{
    CompressedBinaryTrie<std::uint32_t> trie; // 既定で32bit。30bitに絞るならCompressedBinaryTrie<std::uint32_t, 29>。
    trie.reserve(100); // 異なる値の予定数。
    trie.add(5, 10);
    trie.add(5);
    std::cout << trie.insert_unique(2, 20) << '\n'; // 1
    std::cout << trie.insert_unique(2, 99) << '\n'; // 0（IDも変更しない）
    trie.operate_xor(3); // 集合は {1, 6, 6}。
    std::cout << trie.min_element().first << '\n'; // 1
    std::cout << trie.max_element().first << '\n'; // 6
    std::cout << trie.get_kth(2).first << '\n';     // 6（1始まり）
    {
        const auto [count, ids] = trie.find(6);
        std::cout << count << '\n'; // 2
        for (int id : ids)
            std::cout << id << '\n'; // 10
    } // ID参照は更新をまたいで保持しない。
    std::cout << trie.del(6, 10) << '\n'; // 1
    std::cout << trie.find(6).first << '\n'; // 1
    std::cout << trie.del(100) << '\n'; // 0

    // 集合は {1, 6}。追加クエリは集合の状態を変更しない。
    std::cout << trie.count(6) << '\n';          // 1
    std::cout << trie.contains(2) << '\n';       // 0
    std::cout << trie.count_less(6) << '\n';     // 1
    const auto [xor_value, ids] = trie.min_xor(3);
    std::cout << xor_value << '\n';             // 2（3 ^ 1）
    for (int id : ids)
        std::cout << id << '\n';                // 20（保存値1のID）
    std::cout << trie.max_xor(3).first << '\n';   // 5（3 ^ 6）
    std::cout << trie.count_xor_less(3, 5) << '\n'; // 1
    // 64bitの共通接頭辞を持つ集合。接頭辞の外にある値は存在しない。
    CompressedBinaryTrie<std::uint64_t> prefix;
    const auto high = std::uint64_t{1} << 63;
    prefix.add(high, 1);
    prefix.add(high + 1, 2);
    std::cout << prefix.contains(high + 1) << ' ' << prefix.del(0) << '\n'; // 1 0
}
