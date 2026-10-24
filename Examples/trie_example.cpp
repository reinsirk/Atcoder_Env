// SPDX-License-Identifier: CC0-1.0
#include "../String/Trie.cpp"
#include <iostream>
#include <memory>
#include <string>
#include <utility>

int main() {
    Trie trie;
    trie.reserve_nodes(32);
    trie.insert("apple", 2);
    trie.insert("app");
    trie.insert("banana");
    trie.insert(""); // 空文字列も1語。
    std::cout << trie.size() << ' ' << trie.distinct_size() << '\n'; // 5 4
    std::cout << trie.count("apple") << ' ' << trie.contains("app") << '\n'; // 2 1
    std::cout << trie.prefix_count("app") << ' ' << trie.starts_with("ba") << '\n'; // 3 1
    std::cout << *trie.longest_prefix("application") << '\n'; // 3
    std::cout << trie.count_less("banana") << ' ' << *trie.kth(2) << '\n'; // 4 apple
    std::cout << *trie.lower_bound("apq") << '\n'; // banana
    for (const auto& [word, count] : trie.items("app", 2))
        std::cout << word << ':' << count << '\n'; // app:1 / apple:2

    trie.erase("apple");
    std::cout << trie.erase_all("apple") << '\n'; // 1
    const std::string bytes("a\0b", 3);
    trie.insert(bytes); // NULを含む全256種のバイトに対応。
    std::cout << trie.contains(bytes) << '\n'; // 1
    Trie copy = trie; // コピーは独立。
    copy.clear();
    swap(copy, trie);
    std::cout << trie.empty() << ' ' << copy.node_count() << '\n'; // 1 12

    TrieNode root; // 従来の子mapも利用可。こちらのコピーは子を共有する。
    root.children['a'] = std::make_shared<TrieNode>();
}
