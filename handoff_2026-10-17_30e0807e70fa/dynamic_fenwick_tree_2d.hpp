// SPDX-License-Identifier: CC0-1.0
// Original source: https://nyaannyaan.github.io/library/data-structure-2d/dynamic-binary-indexed-tree-2d.hpp
// Modified by reinsirk with assistance from OpenAI Codex.
#ifndef REINSIRK_DYNAMIC_FENWICK_TREE_2D_HPP
#define REINSIRK_DYNAMIC_FENWICK_TREE_2D_HPP
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef REINSIRK_HASH_MAP_DEFINED
#define REINSIRK_HASH_MAP_DEFINED
// 整数キー用。要素への参照・ポインタは再確保で無効になる。
template <class Key, class Val>
class HashMap {
    static_assert(std::is_integral<Key>::value &&
                  !std::is_same<typename std::remove_cv<Key>::type, bool>::value &&
                  sizeof(Key) <= sizeof(std::uint64_t),
                  "HashMap: Key must be an integer of at most 64 bits");
    struct Value { Val value{}; };
    std::vector<Key> keys_;
    std::vector<Value> values_;
    std::vector<std::uint8_t> used_;
    std::size_t size_ = 0;
    Val default_value_{};

    static std::uint64_t mix(std::uint64_t x) noexcept {
        x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
        x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
        return x ^ (x >> 31);
    }

    static std::size_t hash(Key key) noexcept {
        static const std::uint64_t salt = mix(static_cast<std::uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count()));
        return static_cast<std::size_t>(mix(static_cast<std::uint64_t>(key) + salt));
    }

    std::size_t max_capacity() const noexcept {
        return std::min({keys_.max_size(), values_.max_size(), used_.max_size()});
    }

    void rehash(std::size_t capacity) {
        if (capacity > max_capacity())
            throw std::length_error("HashMap: capacity overflow");
        std::vector<Key> keys(capacity);
        std::vector<Value> values(capacity);
        std::vector<std::uint8_t> used(capacity, 0);
        for (std::size_t i = 0; i < used_.size(); ++i) {
            if (!used_[i]) continue;
            std::size_t p = hash(keys_[i]) & (capacity - 1);
            while (used[p]) p = (p + 1) & (capacity - 1);
            keys[p] = keys_[i];
            values[p].value = values_[i].value;
            used[p] = 1;
        }
        keys_.swap(keys);
        values_.swap(values);
        used_.swap(used);
    }

public:
    // 空の状態では配列を確保しない。
    HashMap() = default;
    HashMap(const HashMap&) = default;
    HashMap(HashMap&& other) : HashMap() { swap(other); }
    HashMap& operator=(HashMap other) {
        swap(other);
        return *this;
    }

    void swap(HashMap& other) {
        using std::swap;
        swap(default_value_, other.default_value_);
        keys_.swap(other.keys_);
        values_.swap(other.values_);
        used_.swap(other.used_);
        swap(size_, other.size_);
    }

    // n要素まで再確保せずに格納できる容量を確保する。
    void reserve(std::size_t n) {
        if (n <= keys_.size() - keys_.size() / 4) return;
        std::size_t capacity = std::max(std::size_t{8}, keys_.size());
        while (n > capacity - capacity / 4) {
            if (capacity > max_capacity() / 2)
                throw std::length_error("HashMap: capacity overflow");
            capacity *= 2;
        }
        rehash(capacity);
    }

    // 容量を2倍にする。空なら初期容量を確保する。
    void reallocate() {
        if (keys_.empty()) {
            rehash(8);
        } else {
            if (keys_.size() > max_capacity() / 2)
                throw std::length_error("HashMap: capacity overflow");
            rehash(keys_.size() * 2);
        }
    }

    // 未登録なら設定済みの初期値で追加する。
    Val& operator[](Key key) {
        if (keys_.empty()) reallocate();
        std::size_t p = hash(key) & (keys_.size() - 1);
        while (used_[p]) {
            if (keys_[p] == key) return values_[p].value;
            p = (p + 1) & (keys_.size() - 1);
        }
        if (size_ == keys_.size() - keys_.size() / 4) {
            reallocate();
            p = hash(key) & (keys_.size() - 1);
            while (used_[p]) p = (p + 1) & (keys_.size() - 1);
        }
        keys_[p] = key;
        values_[p].value = default_value_;
        used_[p] = 1;
        ++size_;
        return values_[p].value;
    }

    // 未登録ならnullptrを返す。読取では要素を追加しない。
    const Val* find(const Key& key) const noexcept {
        if (keys_.empty()) return nullptr;
        std::size_t p = hash(key) & (keys_.size() - 1);
        while (used_[p]) {
            if (keys_[p] == key) return &values_[p].value;
            p = (p + 1) & (keys_.size() - 1);
        }
        return nullptr;
    }
    Val* find(const Key& key) noexcept {
        return const_cast<Val*>(static_cast<const HashMap&>(*this).find(key));
    }

    // 登録済みのキーと値を列挙する。順序は不定。
    std::vector<std::pair<Key, Val>> enumerate() const {
        std::vector<std::pair<Key, Val>> result;
        result.reserve(size_);
        for (std::size_t i = 0; i < used_.size(); ++i)
            if (used_[i]) result.emplace_back(keys_[i], values_[i].value);
        return result;
    }

    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    // 今後追加する要素の初期値を設定する。
    void set_default(const Val& value) { default_value_ = value; }
};

#endif  // REINSIRK_HASH_MAP_DEFINED

#ifndef REINSIRK_DYNAMIC_FENWICK_TREE_DEFINED
#define REINSIRK_DYNAMIC_FENWICK_TREE_DEFINED
// 動的1次元Fenwick木。size>=0、添字は[0, size)、Tの加減算は表現範囲内。
// 入力範囲の実行時チェックは行わない。
template <typename S, typename T>
class DynamicFenwickTree {
  static_assert(std::is_integral_v<S> && !std::is_same_v<S, bool>,
                "DynamicFenwickTree: integral index required");
  using U = std::make_unsigned_t<S>;
  static_assert(std::numeric_limits<U>::digits <= 64, "index exceeds 64 bits");
  U n_ = 0;
  HashMap<U, T> data_;

  static U lowbit(U x) noexcept { return x & static_cast<U>(U{0} - x); }
  T range_sum(U left, U right) const {
    T result{};
    while (left != right) {
      if (left < right) {
        if (const T* value = data_.find(right)) result += *value;
        right -= lowbit(right);
      } else {
        if (const T* value = data_.find(left)) result -= *value;
        left -= lowbit(left);
      }
    }
    return result;
  }

 public:
  // 既定では長さ0。初期化時にセルを確保しない。
  explicit DynamicFenwickTree(S size = 0) : n_(static_cast<U>(size)) {}
  // 配列長と、確保済みの内部セル数。
  S size() const noexcept { return static_cast<S>(n_); }
  bool empty() const noexcept { return n_ == 0; }
  std::size_t stored_nodes() const noexcept { return data_.size(); }
  // 内部セル数を指定してハッシュ表を事前確保する。
  void reserve(std::size_t nodes) { data_.reserve(nodes); }
  // k番目にxを加算する。0<=k<size()。
  void add(S k, const T& x) {
    U index = static_cast<U>(k);
    for (++index;;) {
      data_[index] += x;
      const U step = lowbit(index);
      if (step > n_ - index) break;
      index += step;
    }
  }
  // [0, k)の和。0<=k<=size()。検索ではセルを追加しない。
  T sum(S k) const { return range_sum(0, static_cast<U>(k)); }
  // [left, right)の和。0<=left<=right<=size()。
  T sum(S left, S right) const {
    return range_sum(static_cast<U>(left), static_cast<U>(right));
  }
  // k番目の値。0<=k<size()。
  T operator[](S k) const {
    const U index = static_cast<U>(k);
    return range_sum(index, index + U{1});
  }
  // 全要素が非負のとき、sum(k+1)>=wとなる最小k。w<=0は0、不足時はsize()。
  S lower_bound(T w) const {
    if (!(T{} < w)) return 0;
    U index = 0, step = 1;
    while (step <= n_ / 2) step *= 2;
    for (; step != 0; step >>= 1) {
      if (step > n_ - index) continue;
      const U next = index + step;
      const T* value = data_.find(next);
      if (!value) index = next;
      else if (*value < w) { w -= *value; index = next; }
    }
    return static_cast<S>(index);
  }
};

#endif  // REINSIRK_DYNAMIC_FENWICK_TREE_DEFINED

#ifndef REINSIRK_DYNAMIC_FENWICK_TREE_2D_DEFINED
#define REINSIRK_DYNAMIC_FENWICK_TREE_2D_DEFINED
// 動的2次元Fenwick木。rows,cols>=0、添字は[0, rows)x[0, cols)。
// 入力範囲の実行時チェックなし。Tの加減算は表現範囲内であること。
template <typename T>
class DynamicFenwickTree2D {
  using U = std::uint32_t;
  U rows_ = 0, cols_ = 0;
  HashMap<std::uint64_t, T> data_;

  static U lowbit(U x) noexcept { return x & (U{0} - x); }
  static std::uint64_t key(U row, U col) noexcept {
    return (static_cast<std::uint64_t>(row) << 32) | col;
  }
  T row_sum(U row, U left, U right) const {
    T result{};
    while (left != right) {
      if (left < right) {
        if (const T* value = data_.find(key(row, right))) result += *value;
        right -= lowbit(right);
      } else {
        if (const T* value = data_.find(key(row, left))) result -= *value;
        left -= lowbit(left);
      }
    }
    return result;
  }

 public:
  // 既定では0x0。領域の大きさによらず初期化はO(1)。
  explicit DynamicFenwickTree2D(int rows = 0, int cols = 0)
      : rows_(static_cast<U>(rows)), cols_(static_cast<U>(cols)) {}
  // 行数・列数・確保済みの内部セル数。
  int rows() const noexcept { return static_cast<int>(rows_); }
  int cols() const noexcept { return static_cast<int>(cols_); }
  std::size_t stored_nodes() const noexcept { return data_.size(); }
  // 内部セル数を指定してハッシュ表を事前確保する。
  void reserve(std::size_t nodes) { data_.reserve(nodes); }
  // (row, col)にxを加算。0<=row<rows()、0<=col<cols()。
  void add(int row, int col, const T& x) {
    for (U i = static_cast<U>(row) + 1;;) {
      for (U j = static_cast<U>(col) + 1;;) {
        data_[key(i, j)] += x;
        const U step = lowbit(j);
        if (step > cols_ - j) break;
        j += step;
      }
      const U step = lowbit(i);
      if (step > rows_ - i) break;
      i += step;
    }
  }
  // [0, row)x[0, col)の和。0<=row<=rows()、0<=col<=cols()。
  T sum(int row, int col) const { return sum(0, 0, row, col); }
  // [top, bottom)x[left, right)の和。0<=top<=bottom<=rows()、列も同様。
  T sum(int top, int left, int bottom, int right) const {
    if (top == bottom || left == right) return T{};
    U lo = static_cast<U>(top), hi = static_cast<U>(bottom);
    const U l = static_cast<U>(left), r = static_cast<U>(right);
    T result{};
    while (lo != hi) {
      if (lo < hi) { result += row_sum(hi, l, r); hi -= lowbit(hi); }
      else { result -= row_sum(lo, l, r); lo -= lowbit(lo); }
    }
    return result;
  }
};

#endif  // REINSIRK_DYNAMIC_FENWICK_TREE_2D_DEFINED
#endif  // REINSIRK_DYNAMIC_FENWICK_TREE_2D_HPP

