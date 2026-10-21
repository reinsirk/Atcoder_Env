// SPDX-License-Identifier: CC0-1.0
// Original Source: https://nyaannyaan.github.io/library/data-structure/line-container.hpp.html
// Original Source: https://github.com/kth-competitive-programming/kactl/blob/main/content/data-structures/LineContainer.h
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>

#ifndef REINSIRK_LINE_CONTAINER_DEFINED
#define REINSIRK_LINE_CONTAINER_DEFINED

enum Objective { MAXIMIZE = +1, MINIMIZE = -1 };

// k,m: 元の係数。p: この直線が選ばれる右端(整数は型の範囲に制限)。
template <typename T>
struct Line {
  mutable T k, m, p;
  bool operator<(const Line& other) const { return k < other.k; }
  bool operator<(T x) const { return p < x; }
};

// lc_inf<T>(): 整数では最大値、浮動小数点では正の無限大を返す。
template <typename T>
constexpr T lc_inf() {
  if constexpr (std::is_floating_point_v<T>) return std::numeric_limits<T>::infinity();
  else return std::numeric_limits<T>::max();
}

// lc_div(a,b): 整数は床除算、浮動小数点は通常除算。0除算・整数結果の範囲外は例外。
template <typename T>
T lc_div(T a, T b) {
  if (b == 0) throw std::domain_error("lc_div: division by zero");
  if constexpr (std::is_integral_v<T>) {
    static_assert(std::is_signed_v<T>);
    if (a == std::numeric_limits<T>::min() && b == -1)
      throw std::overflow_error("lc_div: quotient overflow");
    return static_cast<T>(a / b - ((a < 0) != (b < 0) && a % b != 0));
  } else return a / b;
}

namespace reinsirk::detail {
template <typename T, Objective objective>
struct LineComparator {
  using is_transparent = void;
  bool operator()(const Line<T>& a, const Line<T>& b) const {
    if constexpr (objective == MAXIMIZE) return a.k < b.k;
    else return a.k > b.k;
  }
  bool operator()(const Line<T>& a, T x) const { return a < x; }
  bool operator()(T x, const Line<T>& a) const { return x < a.p; }
};
}  // namespace reinsirk::detail

// add(k,m): y=k*x+mを追加、償却O(log h)。整数は符号付き64bit以下、浮動係数は有限値。
// query(x): 最適値をO(log h)で返す。空は検査時例外、省略時は非空が前提。整数範囲外は常に例外。
// query_optional(x): 空ならnullopt、それ以外はqueryと同じ。xは浮動小数点なら有限値。
// query_line(x): 選択した元の(k,m)をO(log h)で返す。同点の係数は未規定。
// query_line_optional(x): 空ならnullopt、それ以外はquery_lineと同じ。
// size()/empty()/clear(): 有効な包絡直線の本数・空判定・全削除、O(1)/O(1)/O(h)。
// begin()/end()/lower_bound(x): 元係数と交点を参照。最小化の傾き順は降順。直接変更は不変条件を壊す。
// insect(x,y): 旧互換の内部交点更新。正しい包絡iterator以外では呼び出さない。
// コピーは独立した包絡。hは有効直線数。浮動小数点版の交点・最適値には丸め誤差がある。
template <typename T, Objective objective>
struct LineContainer : std::multiset<Line<T>, reinsirk::detail::LineComparator<T, objective>> {
  static_assert((std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) <= 8) ||
                std::is_floating_point_v<T>);
  static_assert(objective == MAXIMIZE || objective == MINIMIZE);
  using super = std::multiset<Line<T>, reinsirk::detail::LineComparator<T, objective>>;
  using super::begin, super::end, super::insert, super::erase;
  using super::empty, super::lower_bound;
  static constexpr T inf = lc_inf<T>();

 private:
  static bool boundary_less(const Line<T>& a, const Line<T>& b) {
    return a.p < b.p;
  }
  static bool intersection(Line<T> const& a, Line<T> const& b) {
    if constexpr (std::is_integral_v<T>) {
      using U = std::make_unsigned_t<T>;
      const bool negative_n = b.m < a.m, negative_d = a.k < b.k;
      const U n = negative_n ? U(a.m) - U(b.m) : U(b.m) - U(a.m);
      const U d = negative_d ? U(b.k) - U(a.k) : U(a.k) - U(b.k);
      U q = n / d;
      constexpr U top = U(std::numeric_limits<T>::max());
      if (negative_n != negative_d) {
        q = static_cast<U>(q + U(n % d != 0));
        if (q >= top + U{1}) {
          a.p = std::numeric_limits<T>::min();
          return q > top + U{1};
        }
        else a.p = static_cast<T>(-static_cast<T>(q));
      } else a.p = q > top ? inf : static_cast<T>(q);
      return false;
    } else {
      const T n = b.m - a.m, d = a.k - b.k;
      if (std::isfinite(n + d) || (std::isfinite(n) && std::isfinite(d))) a.p = n / d;
      else if (!std::isfinite(n) && !std::isfinite(d))
        a.p = (b.m / 2 - a.m / 2) / (a.k / 2 - b.k / 2);
      else if (!std::isfinite(n)) a.p = ((b.m / 2 - a.m / 2) / d) * 2;
      else a.p = (n / (a.k / 2 - b.k / 2)) / 2;
      return false;
    }
  }
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((always_inline))
#endif
  static T evaluate(T k, T m, T x) {
    if constexpr (std::is_floating_point_v<T>) return k * x + m;
    else if constexpr (sizeof(T) < 8) {
      const std::int64_t result = std::int64_t(k) * x + m;
      if (result < std::numeric_limits<T>::min() || result > std::numeric_limits<T>::max())
        throw std::overflow_error("LineContainer: value overflow");
      return static_cast<T>(result);
    } else {
#if defined(__SIZEOF_INT128__) && !defined(REINSIRK_LINE_CONTAINER_FORCE_PORTABLE)
      T fast;
      if (!__builtin_mul_overflow(k, x, &fast) && !__builtin_add_overflow(fast, m, &fast))
#if defined(__clang__)
        [[likely]]
#endif
        return fast;
      __extension__ using Wide = __int128;
      const Wide result = Wide(k) * x + m;
      if (result < std::numeric_limits<T>::min() || result > std::numeric_limits<T>::max())
        throw std::overflow_error("LineContainer: value overflow");
      return static_cast<T>(result);
#else
      using U = std::uint64_t;
      // 32bit範囲なら積と和が64bitに収まる。
      constexpr U narrow_offset = U{1} << 31;
      if (((U(k) + narrow_offset) | (U(x) + narrow_offset) |
           (U(m) + narrow_offset)) < (U{1} << 32))
        return k * x + m;
      const U a = k < 0 ? U{0} - U(k) : U(k);
      const U b = x < 0 ? U{0} - U(x) : U(x);
      const U c = m < 0 ? U{0} - U(m) : U(m);
      constexpr U mask = 0xffffffffU;
      const U t0 = (a & mask) * (b & mask);
      const U t1 = (a >> 32) * (b & mask) + (t0 >> 32);
      const U t2 = (a & mask) * (b >> 32) + (t1 & mask);
      U hi = (a >> 32) * (b >> 32) + (t1 >> 32) + (t2 >> 32);
      U lo = (t2 << 32) | (t0 & mask);
      bool negative = (k < 0) != (x < 0);
      if (negative == (m < 0)) {
        const U previous = lo;
        lo += c;
        hi += lo < previous;
      } else if (hi != 0 || lo >= c) {
        const U previous = lo;
        lo -= c;
        hi -= previous < c;
      } else { lo = c - lo; negative = m < 0; }
      const U limit = U(std::numeric_limits<T>::max()) + U(negative);
      if (hi != 0 || lo > limit) throw std::overflow_error("LineContainer: value overflow");
      if (negative && lo == U(std::numeric_limits<T>::max()) + U{1})
        return std::numeric_limits<T>::min();
      return negative ? -static_cast<T>(lo) : static_cast<T>(lo);
#endif
    }
  }
  static void check_x(T x) {
#ifndef SKIP_RUNTIME_CHECKS
    if constexpr (std::is_floating_point_v<T>)
      if (!std::isfinite(x)) throw std::invalid_argument("LineContainer: nonfinite x");
#else
    (void)x;
#endif
  }
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((always_inline))
#endif
  typename super::const_iterator selected(T x) const {
    check_x(x);
#ifndef SKIP_RUNTIME_CHECKS
    if (empty()) throw std::out_of_range("LineContainer: empty query");
#endif
    return lower_bound(x);
  }

  bool insect_impl(typename super::iterator x, typename super::iterator y, bool& below) {
    below = false;
    if (y == end()) { x->p = inf; return false; }
    if (x->k == y->k) {
      const bool better = objective == MAXIMIZE ? x->m > y->m : x->m < y->m;
      if constexpr (std::is_integral_v<T>) {
        x->p = better ? inf : std::numeric_limits<T>::min();
        below = !better;
      }
      else x->p = better ? inf : -inf;
    } else below = intersection(*x, *y);
    return !below && !boundary_less(*x, *y);
  }

 public:
  bool insect(typename super::iterator x, typename super::iterator y) {
    bool below;
    return insect_impl(x,y,below);
  }
  void add(T k, T m) {
#ifndef SKIP_RUNTIME_CHECKS
    if constexpr (std::is_floating_point_v<T>)
      if (!std::isfinite(k) || !std::isfinite(m))
        throw std::invalid_argument("LineContainer: nonfinite coefficient");
#endif
    add_unchecked(k, m);
  }

 private:
  friend struct LineContainer2D;
  void add_unchecked(T k, T m) {
    auto z = insert({k, m, 0}), y = z++, x = y;
    bool below;
    while (insect_impl(y, z, below)) z = erase(z);
    if constexpr (std::is_integral_v<T>)
      if (below) { erase(y); return; }
    if (x != begin() && insect(--x, y)) insect(x, y = erase(y));
    while ((y = x) != begin() && !boundary_less(*--x, *y)) insect(x, erase(y));
    // floorが最小値の直線はその点で最適になり得る。真の域外だけ除く。
    if constexpr (std::is_integral_v<T>) {
      const auto first = begin();
      if (first->p == std::numeric_limits<T>::min()) [[unlikely]] {
        const auto next = std::next(first);
        if (next != end()) {
          bool first_below;
          insect_impl(first, next, first_below);
          if (first_below) erase(first);
        }
      }
    }
  }
 public:
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((always_inline))
#endif
  T query(T x) const {
    if constexpr (std::is_integral_v<T>) {
#ifndef SKIP_RUNTIME_CHECKS
      if (empty()) throw std::out_of_range("LineContainer: empty query");
#endif
      const auto first = begin();
      if (x <= first->p) [[unlikely]] return evaluate(first->k, first->m, x);
      const auto line = lower_bound(x);
      return evaluate(line->k, line->m, x);
    } else {
      const auto line = selected(x);
      return evaluate(line->k, line->m, x);
    }
  }
  std::optional<T> query_optional(T x) const {
    check_x(x);
    if (empty()) return std::nullopt;
    const auto line = lower_bound(x);
    return evaluate(line->k, line->m, x);
  }
  std::pair<T,T> query_line(T x) const {
    const auto line = selected(x);
    return {line->k, line->m};
  }
  std::optional<std::pair<T,T>> query_line_optional(T x) const {
    check_x(x);
    if (empty()) return std::nullopt;
    const auto line = lower_bound(x);
    return std::pair<T,T>{line->k, line->m};
  }
};
template <typename T> using MinLineContainer = LineContainer<T, MINIMIZE>;
template <typename T> using MaxLineContainer = LineContainer<T, MAXIMIZE>;
#endif
