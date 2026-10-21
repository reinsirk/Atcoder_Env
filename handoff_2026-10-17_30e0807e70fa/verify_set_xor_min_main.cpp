#include <cstdint>
#include <iostream>

// ここにBinaryTrieのライブラリを貼り付ける。

int main() {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  int queries;
  if (!(std::cin >> queries)) return 0;
  BinaryTrie<std::uint32_t, 29> trie;  // 0 <= x < 2^30。

  while (queries--) {
    int type;
    std::uint32_t x;
    std::cin >> type >> x;

    if (type == 0) {
      if (trie.find(x).first == 0) trie.add(x);  // 重複追加を防ぐ。
    } else if (type == 1) {
      trie.del(x);  // 未登録なら何もしない。
    } else {
      trie.operate_xor(x);
      const std::uint32_t answer = trie.min_element().first;
      trie.operate_xor(x);  // 元の集合へ戻す。
      std::cout << answer << '\n';
    }
  }
  return 0;
}
