// SPDX-License-Identifier: CC0-1.0
// Based on: https://nyaannyaan.github.io/library/data-structure/binary-trie.hpp
#ifndef REINSIRK_BINARY_TRIE_DEFINED
#define REINSIRK_BINARY_TRIE_DEFINED

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

// 重複を許す非負整数の二分トライ（C++17）。T は bool / const / volatile を除く整数型。
// キーと XOR マスクは 0 <= x < 2^(MAX_LOG + 1) を満たすこと。
// MAX_LOG は最高位ビット番号。NODES は根込みのノード上限。以下 B = MAX_LOG + 1。
// get_kth は重複込み・1始まり。ID は重複可、-1 は匿名（ID一覧には含めない）。
// ID省略の削除は匿名を優先し、なければ任意のID付き要素を削除する。
// ID一覧は順序不定の読み取り専用参照。次の更新まで有効（破棄後も無効）。
template <typename T, int MAX_LOG, std::size_t NODES = 16777216>
class BinaryTrie {
  static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>,
                "T must be an integer type other than bool");
  static_assert(std::is_same_v<T, std::remove_cv_t<T>>,
                "T must not be const or volatile");
  static_assert(MAX_LOG >= 0 && MAX_LOG < std::numeric_limits<T>::digits,
                "MAX_LOG is outside the nonnegative range of T");
  static_assert(NODES > 0, "NODES must include at least the root");

 public:
  using Container = std::vector<int>;
  using size_type = std::size_t;
  using FindResult = std::pair<size_type, const Container&>;
  using ElementResult = std::pair<T, const Container&>;

 private:
  using U = std::make_unsigned_t<T>;
  static constexpr size_type kBits = MAX_LOG + 1;
  static constexpr size_type kBlockSize = 4096;

  struct Node {
    Node* child[2] = {nullptr, nullptr};  // blocks_ 内への非所有ポインタ。
    size_type count = 0;
    std::unique_ptr<Container> ids;  // ID配列は必要な葉のみ確保。
  };

  // ノードをブロック単位で所有し、拡張時もアドレスを維持する。
  std::vector<std::unique_ptr<Node[]>> blocks_;
  size_type capacity_ = 0;
  size_type next_unused_ = 0;
  Node* free_list_ = nullptr;
  Node* root_ = nullptr;
  U lazy_ = 0;

  // 再利用リストか未使用領域から取得し、必要なら上限内でブロックを確保する。
  Node* acquire_node() {
    if (free_list_) {
      Node* node = free_list_;
      free_list_ = node->child[0];
      node->child[0] = node->child[1] = nullptr;
      return node;
    }
    if (next_unused_ == capacity_) {
      if (capacity_ == NODES) {
        throw std::length_error("BinaryTrie: node limit exceeded");
      }
      const auto n = std::min(kBlockSize, NODES - capacity_);
      blocks_.push_back(std::make_unique<Node[]>(n));
      capacity_ += n;
    }
    const auto index = next_unused_++;
    return &blocks_[index / kBlockSize][index % kBlockSize];
  }

  // ノードを初期化して再利用リストへ戻す。子の回収は呼び出し側で行う。
  void release_node(Node* node) noexcept {
    node->ids.reset();
    node->count = 0;
    node->child[0] = free_list_;
    node->child[1] = nullptr;
    free_list_ = node;
  }

  // 下から i 番目のビットを取り出す（0 <= i <= MAX_LOG）。
  static unsigned bit(U x, int i) noexcept {
    return static_cast<unsigned>((x >> i) & U{1});
  }

  // 読み取り専用のID一覧を返す。ノードやIDがなければ空配列。
  static const Container& ids_of(const Node* node) noexcept {
    static const Container empty;
    return node && node->ids ? *node->ids : empty;
  }

  // 内部キー（公開キー ^ lazy_）の葉を探す。未登録なら nullptr。
  const Node* find_leaf(U stored_key) const noexcept {
    const Node* node = root_;
    for (int i = MAX_LOG; i >= 0 && node; --i) {
      node = node->child[bit(stored_key, i)];
    }
    return node;
  }

  // XOR反映後の最小値・最大値を選ぶ共通処理（O(B)）。
  ElementResult extreme(bool maximum) const {
    const Node* node = root_;
    U value = 0;
    for (int i = MAX_LOG; i >= 0; --i) {
      const unsigned logical_bit = static_cast<unsigned>(maximum);
      const unsigned preferred = bit(lazy_, i) ^ logical_bit;
      const bool available = node->child[preferred] != nullptr;
      const unsigned chosen = available ? preferred : (preferred ^ 1u);
      if ((chosen ^ bit(lazy_, i)) != 0) value |= U{1} << i;
      node = node->child[chosen];
    }
    return {static_cast<T>(value), ids_of(node)};
  }

 public:
  // 空の集合を構築し、最初のブロックだけを確保する。
  BinaryTrie() : root_(acquire_node()) {}

  // コピーを禁止する。
  BinaryTrie(const BinaryTrie&) = delete;
  // コピー代入を禁止する。
  BinaryTrie& operator=(const BinaryTrie&) = delete;
  // ムーブを禁止する。
  BinaryTrie(BinaryTrie&&) = delete;
  // ムーブ代入を禁止する。
  BinaryTrie& operator=(BinaryTrie&&) = delete;

  // 重複・匿名要素を含む要素数を返す（O(1)）。
  [[nodiscard]] size_type size() const noexcept { return root_->count; }
  // 空集合なら true（O(1)）。
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  // x を1個追加する（O(B)、ID追加は償却 O(1)）。失敗時も値・件数・IDを保持する。
  // 件数超過は overflow_error、容量超過は length_error、確保失敗は bad_alloc。
  void add(T x, int id = -1) {
    const U key = static_cast<U>(x) ^ lazy_;
    if (size() == std::numeric_limits<size_type>::max()) {
      throw std::overflow_error("BinaryTrie: element count overflow");
    }
    std::array<Node*, kBits + 1> path{};
    path[0] = root_;
    size_type first_new = 0;
    size_type last_new = 0;
    try {
      for (size_type depth = 1; depth <= kBits; ++depth) {
        const unsigned b = bit(key, MAX_LOG - static_cast<int>(depth - 1));
        Node* child = path[depth - 1]->child[b];
        if (!child) {
          child = acquire_node();
          if (!first_new) {
            first_new = depth;  // 成功するまで新しい部分木を接続しない。
          } else {
            path[depth - 1]->child[b] = child;
          }
          last_new = depth;
        }
        path[depth] = child;
      }
      Node* leaf = path[kBits];
      if (id != -1) {
        if (leaf->ids) {
          leaf->ids->push_back(id);
        } else {
          auto ids = std::make_unique<Container>();
          ids->push_back(id);
          leaf->ids = std::move(ids);
        }
      }
    } catch (...) {
      if (first_new) {
        for (size_type depth = last_new; depth >= first_new; --depth) {
          release_node(path[depth]);
        }
      }
      throw;
    }
    if (first_new) {
      const auto b = bit(key, MAX_LOG - static_cast<int>(first_new - 1));
      path[first_new - 1]->child[b] = path[first_new];
    }
    for (Node* node : path) ++node->count;
  }

  // x を1個削除する。値または指定IDがなければ false。
  // ID省略は O(B)、指定ありは O(B + xに付いたID数)。
  bool del(T x, int id = -1) {
    const U key = static_cast<U>(x) ^ lazy_;
    std::array<Node*, kBits + 1> path{};
    path[0] = root_;
    for (size_type depth = 1; depth <= kBits; ++depth) {
      const auto b = bit(key, MAX_LOG - static_cast<int>(depth - 1));
      path[depth] = path[depth - 1]->child[b];
      if (!path[depth]) return false;
    }
    Node* leaf = path[kBits];
    if (id != -1) {
      if (!leaf->ids) return false;
      auto& ids = *leaf->ids;
      const auto it = std::find(ids.begin(), ids.end(), id);
      if (it == ids.end()) return false;
      *it = ids.back();
      ids.pop_back();
    } else if (leaf->ids && leaf->count == leaf->ids->size()) {
      leaf->ids->pop_back();
    }
    for (Node* node : path) --node->count;
    for (size_type depth = kBits; depth > 0; --depth) {
      if (path[depth]->count != 0) break;
      const auto b = bit(key, MAX_LOG - static_cast<int>(depth - 1));
      path[depth - 1]->child[b] = nullptr;
      release_node(path[depth]);
    }
    return true;
  }

  // {件数, ID一覧} を返す（O(B)）。未登録なら {0, 空配列}。
  // 件数には匿名要素も含む。
  [[nodiscard]] FindResult find(T x) const {
    const Node* leaf = find_leaf(static_cast<U>(x) ^ lazy_);
    return {leaf ? leaf->count : 0, ids_of(leaf)};
  }

  // {最小値, ID一覧} を返す（O(B)）。空でないこと。
  [[nodiscard]] ElementResult min_element() const { return extreme(false); }

  // {最大値, ID一覧} を返す（O(B)）。空でないこと。
  [[nodiscard]] ElementResult max_element() const { return extreme(true); }

  // {昇順でk番目の値, その値の全ID一覧} を返す（O(B)）。
  // 1 <= k <= size() を満たすこと。
  [[nodiscard]] ElementResult get_kth(std::int64_t k) const {
    auto rank = static_cast<size_type>(k);
    const Node* node = root_;
    U value = 0;
    for (int i = MAX_LOG; i >= 0; --i) {
      const auto zero = bit(lazy_, i);
      const Node* left = node->child[zero];
      const size_type count = left ? left->count : 0;
      if (rank <= count) {
        node = left;
      } else {
        rank -= count;
        value |= U{1} << i;
        node = node->child[zero ^ 1u];
      }
    }
    return {static_cast<T>(value), ids_of(node)};
  }

  // 全要素に「値 ^= x」を適用する（O(1)）。以後 add(y) で追加する値は y。
  void operate_xor(T x) { lazy_ ^= static_cast<U>(x); }
};

#endif  // REINSIRK_BINARY_TRIE_DEFINED
