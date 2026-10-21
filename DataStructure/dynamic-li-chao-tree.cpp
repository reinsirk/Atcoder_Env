// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/segment-tree/dynamic-li-chao-tree.hpp

#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef SIRKIT_DYNAMIC_LI_CHAO_TREE_DEFINED
#define SIRKIT_DYNAMIC_LI_CHAO_TREE_DEFINED
// 整数座標の閉区間で直線・線分の最小値を管理する。Tは符号付き64ビット以下の整数。
// 追加する線の対象区間では、数学的なa*x+bがTの範囲内かつINF未満であること。
// Wを座標数とすると直線追加・取得はO(log W)、線分追加はO(log² W)。
// queryは読み取り専用。SKIP_RUNTIME_CHECKSで座標・端点値の検査を除く。
template <typename T, T INF>
class DynamicLiChaoTree {
  // DynamicLiChaoTree(xmin, xmax): 整数の閉区間[xmin, xmax]で空の木を作る。
  // update(a, b) / add_line(a, b): 定義域全体にy=a*x+bを追加する。
  // update_line_segment(a, b, left, right) / add_segment(...): 閉区間[left, right]に線分を追加。left>rightは無操作。
  // query(x): xでの最小値を返す。被覆する線がなければINF。
  // query_line(x): 最小値を与えるLineを返す。同値時は任意、非被覆ならnullopt。
  // empty(): 追加した直線・空でない線分がなければtrue。O(1)。
  // clear(): 全ての線を消し、確保した容量を再利用する。O(格納ノード数)。
  // reserve(n): nノード分を事前確保する。直線本数とは異なる。
  // stored_nodes(): 現在の格納ノード数を返す。取得操作では増えない。O(1)。
  // domain_min() / domain_max(): 定義域の左端・右端を返す。O(1)。
 public:
  struct Line {
    // Line(a, b): y=a*x+b。既定値は空を表す定数INF。
    // get(x): a*x+bを返す。値がTで表現可能であること。
    // over(other, x): xでotherより小さい値を与えるときtrue。
    T slope = 0;
    T intercept = INF;
    Line() = default;
    Line(T a, T b) : slope(a), intercept(b) {}
    T get(T x) const noexcept {
      using U = std::make_unsigned_t<T>;
      // 中間積がTを超えても、最終値が表現可能なら正確に復元できる。
      const U value = static_cast<U>(std::uint64_t(static_cast<U>(slope)) *
                                         std::uint64_t(static_cast<U>(x)) +
                                     std::uint64_t(static_cast<U>(intercept)));
      return std::bit_cast<T>(value);
    }
    bool over(const Line& other, T x) const noexcept { return get(x) < other.get(x); }
  };

 private:
  static_assert(std::is_integral_v<T> && std::is_signed_v<T> &&
                sizeof(T) <= sizeof(std::uint64_t) &&
                std::is_same_v<T, std::remove_cv_t<T>>);
  static_assert(INF > 0);
  using Index = std::uint32_t;
  static constexpr Index no_child = std::numeric_limits<Index>::max();
  struct Node {
    Line line;
    Index left = no_child;
    Index right = no_child;
  };
  T xmin_, xmax_;
  std::vector<Node> nodes_;

#ifndef SKIP_RUNTIME_CHECKS
  void check_coordinate(T x) const {
    if (x < xmin_ || xmax_ < x) throw std::out_of_range("Li Chao coordinate");
  }
  static void check_value(const Line& line, T x) {
    using U = std::make_unsigned_t<T>;
    const U a = line.slope < 0 ? U{} - U(line.slope) : U(line.slope);
    const U v = x < 0 ? U{} - U(x) : U(x);
    // 積そのものを作らず、64ビット全域の値と相殺を含むa*x+bを検査する。
    const auto product_at_most = [a, v](U limit) {
      return a == 0 || v <= limit / a;
    };
    bool valid;
    if ((line.slope < 0) != (x < 0)) {
      valid = product_at_most(U(line.intercept) - U(std::numeric_limits<T>::min())) &&
              (line.intercept < INF || !product_at_most(U(line.intercept) - U(INF)));
    } else {
      valid = line.intercept < INF &&
              product_at_most(U(INF - 1) - U(line.intercept));
    }
    if (!valid) throw std::overflow_error("Li Chao value must fit T and be below INF");
  }
  static void check_line(const Line& line, T left, T right) {
    check_value(line, left);
    if (left != right) check_value(line, right);
  }
#endif

  Index make_node() {
    if (nodes_.size() >= no_child || nodes_.size() == nodes_.max_size())
      throw std::length_error("Li Chao node limit");
    const Index result = static_cast<Index>(nodes_.size());
    nodes_.emplace_back();
    return result;
  }
  Index child(Index index, bool right) {
    Index next = right ? nodes_[index].right : nodes_[index].left;
    if (next == no_child) {
      next = make_node();
      if (right) nodes_[index].right = next;
      else nodes_[index].left = next;
    }
    return next;
  }
  void insert_line(Index index, T left, T right, Line line) {
    for (;;) {
      Node& node = nodes_[index];
      if (node.line.intercept == INF && node.line.slope == 0) {
        node.line = line;
        return;
      }
      const bool better_left = line.over(node.line, left);
      const bool better_right = line.over(node.line, right);
      if (better_left == better_right) {
        if (better_left) node.line = line;
        return;
      }
      const T middle = std::midpoint(left, right);
      const bool better_middle = line.over(node.line, middle);
      if (better_middle) std::swap(node.line, line);
      if (better_left != better_middle) {
        index = child(index, false);
        right = middle;
      } else {
        index = child(index, true);
        left = static_cast<T>(middle + 1);
      }
    }
  }
  void insert_segment(Index index, T left, T right, T qleft, T qright,
                      const Line& line) {
    if (qleft <= left && right <= qright) {
      insert_line(index, left, right, line);
      return;
    }
    const T middle = std::midpoint(left, right);
    if (qleft <= middle)
      insert_segment(child(index, false), left, middle, qleft, qright, line);
    if (middle < qright)
      insert_segment(child(index, true), static_cast<T>(middle + 1), right,
                     qleft, qright, line);
  }
  Line best_line(T x) const noexcept {
    Line best;
    if (nodes_.empty()) return best;
    T left = xmin_, right = xmax_;
    Index index = 0;
    T value = INF;
    for (;;) {
      const Node& node = nodes_[index];
      const T candidate = node.line.get(x);
      if (candidate < value) {
        value = candidate;
        best = node.line;
      }
      if (left == right) return best;
      const T middle = std::midpoint(left, right);
      if (x <= middle) {
        index = node.left;
        right = middle;
      } else {
        index = node.right;
        left = static_cast<T>(middle + 1);
      }
      if (index == no_child) return best;
    }
  }

 public:
  DynamicLiChaoTree(T xmin, T xmax) : xmin_(xmin), xmax_(xmax) {
#ifndef SKIP_RUNTIME_CHECKS
    if (xmax < xmin) throw std::invalid_argument("Li Chao domain");
#endif
  }
  void update(T a, T b) {
    const Line line(a, b);
#ifndef SKIP_RUNTIME_CHECKS
    check_line(line, xmin_, xmax_);
#endif
    if (nodes_.empty()) make_node();
    insert_line(0, xmin_, xmax_, line);
  }
  void add_line(T a, T b) { update(a, b); }
  void update_line_segment(T a, T b, T left, T right) {
    if (right < left) return;
    const Line line(a, b);
#ifndef SKIP_RUNTIME_CHECKS
    check_coordinate(left);
    check_coordinate(right);
    check_line(line, left, right);
#endif
    if (nodes_.empty()) make_node();
    insert_segment(0, xmin_, xmax_, left, right, line);
  }
  void add_segment(T a, T b, T left, T right) {
    update_line_segment(a, b, left, right);
  }
  T query(T x) const {
#ifndef SKIP_RUNTIME_CHECKS
    check_coordinate(x);
#endif
    return best_line(x).get(x);
  }
  std::optional<Line> query_line(T x) const {
#ifndef SKIP_RUNTIME_CHECKS
    check_coordinate(x);
#endif
    Line result = best_line(x);
    if (result.slope == 0 && result.intercept == INF) return std::nullopt;
    return result;
  }
  bool empty() const noexcept { return nodes_.empty(); }
  void clear() noexcept { nodes_.clear(); }
  void reserve(std::size_t count) {
    if (count > no_child || count > nodes_.max_size())
      throw std::length_error("Li Chao node limit");
    nodes_.reserve(count);
  }
  std::size_t stored_nodes() const noexcept { return nodes_.size(); }
  T domain_min() const noexcept { return xmin_; }
  T domain_max() const noexcept { return xmax_; }
};
#endif
