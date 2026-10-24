// SPDX-License-Identifier: CC0-1.0
// Original Source: https://nyaannyaan.github.io/library/data-structure/line-container-2d.hpp.html
// Original Source: https://github.com/kth-competitive-programming/kactl/blob/main/content/data-structures/LineContainer.h
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/line_container_2d.md
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/line_container.md
//
// API一覧
// [LineContainer2D]
// add(x,y): 有限の点を追加、償却O(log h)。確保失敗時は空に戻して例外を伝える。
// max_ld(a,b)/min_ld(a,b): ax+byの最大/最小をO(log h)で返す。有限係数・非空が前提。
// max_ld/min_ld(整数,整数): 値域がint64_t内の整数型は、型から保証できる有限性・比の検査を省く。
// b/aが浮動型の範囲を超える問い合わせは常にoverflow_error。
// max_ll(a,b)/min_ll(a,b): 浮動計算を±4.1e18に制限し、0方向へ切り捨てる(旧挙動)。
// max_rounded_ll(a,b)/min_rounded_ll(a,b): 同じ範囲制限後、半整数は0から遠い方へ丸める。
// max_optional(a,b)/min_optional(a,b): 空ならnullopt、それ以外はld版と同じ。
// argmax(a,b)/argmin(a,b): 最適な元の点(x,y)をO(log h)で返す。同点の点は未規定。
// max_with_point(a,b)/min_with_point(a,b): 1回の探索でResult{内積値,x,y}を返す。ld係数・非空、O(log h)。
// argmax_optional(a,b)/argmin_optional(a,b): 空ならnullopt、それ以外はarg版と同じ。
// size()/empty()/clear(): 登録点数(重複込み)・空判定・全削除、O(1)/O(1)/O(h)。
// swap(other): 2コンテナの全状態をO(1)で交換する。コピーは独立、move後の元は空。
// 公開のminlc/maxlc・座標範囲は参照用。外からの変更は不変条件を壊す。
// long doubleの丸め誤差があり、ll版も厳密な整数内積ではない。hは包絡に残る点数。
// 既定の丸めモード・型本来の精度が前提。ゼロの符号は未規定。
//
// [同梱: LineContainer<T,objective>]
// add(k,m): y=k*x+mを追加、償却O(log h)。整数は符号付き64bit以下、浮動係数は有限値。
// query(x): 最適値をO(log h)で返す。空は検査時例外、省略時は非空が前提。整数範囲外は常に例外。
// query_optional(x): 空ならnullopt、それ以外はqueryと同じ。xは浮動小数点なら有限値。
// query_line(x): 選択した元の(k,m)をO(log h)で返す。同点の係数は未規定。
// query_line_optional(x): 空ならnullopt、それ以外はquery_lineと同じ。
// query_with_line(x): 1回の探索でResult{最適値,k,m}を返す。queryと同じ前提・O(log h)。
// query_with_line_optional(x): 空ならnullopt、それ以外はquery_with_lineと同じ。
// size()/empty()/clear(): 有効な包絡直線の本数・空判定・全削除、O(1)/O(1)/O(h)。
// begin()/end()/lower_bound(x): 元係数と交点を参照。最小化の傾き順は降順。直接変更は不変条件を壊す。
// insect(x,y): 旧互換の内部交点更新。正しい包絡iterator以外では呼び出さない。
// コピーは独立した包絡。hは有効直線数。浮動小数点版の交点・最適値には丸め誤差がある。
// objectiveはMINIMIZE/MAXIMIZE。MinLineContainer<T>/MaxLineContainer<T>は最小化/最大化の別名。
//
// [Line<T>・補助関数]
// Line<T>のk,m: 元の係数。p: この直線が選ばれる右端(整数は型の範囲に制限)。
// lc_inf<T>(): 整数では最大値、浮動小数点では正の無限大を返す。
// lc_div(a,b): 整数は床除算、浮動小数点は通常除算。0除算・整数結果の範囲外は例外。

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

#ifndef SIRKIT_LINE_CONTAINER_DEFINED
#define SIRKIT_LINE_CONTAINER_DEFINED

enum Objective { MAXIMIZE = +1, MINIMIZE = -1 };

template <typename T>
struct Line {
  mutable T k, m, p;
  bool operator<(const Line& other) const { return k < other.k; }
  bool operator<(T x) const { return p < x; }
};

template <typename T>
constexpr T lc_inf() {
  if constexpr (std::is_floating_point_v<T>) return std::numeric_limits<T>::infinity();
  else return std::numeric_limits<T>::max();
}

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

template <typename T, Objective objective>
struct LineContainer : std::multiset<Line<T>, reinsirk::detail::LineComparator<T, objective>> {
  static_assert((std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) <= 8) ||
                std::is_floating_point_v<T>);
  static_assert(objective == MAXIMIZE || objective == MINIMIZE);
  using super = std::multiset<Line<T>, reinsirk::detail::LineComparator<T, objective>>;
  using super::begin, super::end, super::insert, super::erase;
  using super::empty, super::lower_bound;
  static constexpr T inf = lc_inf<T>();
  struct Result { T value, k, m; };

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
#if defined(__SIZEOF_INT128__) && !defined(SIRKIT_LINE_CONTAINER_FORCE_PORTABLE)
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
    if constexpr (std::is_integral_v<T>) {
      // 小さい包絡では、確保前に不要な直線を除く。
      if (super::size() <= 256) {
        Line<T> candidate{k, m, inf};
        auto z = lower_bound(candidate);
        if (z != end() && z->k == k) {
          const bool better = objective == MAXIMIZE ? m > z->m : m < z->m;
          if (!better) return;
        } else if (z != end() && intersection(candidate, *z)) return;
        auto x = z;
        const bool has_previous = z != begin();
        Line<T> previous{};
        // 下端だけで選ばれる直線と、選択域が型の範囲外の直線を区別する。
        bool previous_below = false;
        if (has_previous) {
          --x;
          previous = *x;
          previous_below = intersection(previous, candidate);
          if (!previous_below && previous.p >= candidate.p &&
              (z == end() || candidate.p < z->p)) return;
        }
        auto y = insert(z, candidate);
        while (z != end() && !boundary_less(*y, *z)) {
          z = erase(z);
          insect(y, z);
        }
        if (has_previous) {
          x->p = previous.p;
          if (!previous_below && !boundary_less(*x, *y)) insect(x, y = erase(y));
        } else x = y;
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
        return;
      }
    }
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
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((always_inline))
#endif
  Result query_with_line(T x) const {
    const auto line = selected(x);
    return {evaluate(line->k, line->m, x), line->k, line->m};
  }
#if defined(__GNUC__) || defined(__clang__)
  __attribute__((always_inline))
#endif
  std::optional<Result> query_with_line_optional(T x) const {
    check_x(x);
    if (empty()) return std::nullopt;
    const auto line = lower_bound(x);
    return Result{evaluate(line->k, line->m, x), line->k, line->m};
  }
};
template <typename T> using MinLineContainer = LineContainer<T, MINIMIZE>;
template <typename T> using MaxLineContainer = LineContainer<T, MAXIMIZE>;
#endif

#ifndef SIRKIT_LINE_CONTAINER_2D_DEFINED
#define SIRKIT_LINE_CONTAINER_2D_DEFINED

struct LineContainer2D {
  using ld = long double;
  using ll = std::int64_t;
  struct Result { ld value, x, y; };
  static constexpr ld INF = 4.1e18L;
  static constexpr ll INTEGER_LIMIT = 4'100'000'000'000'000'000;
  ld Xmax = -std::numeric_limits<ld>::infinity();
  ld Xmin = std::numeric_limits<ld>::infinity();
  ld Ymax = -std::numeric_limits<ld>::infinity();
  ld Ymin = std::numeric_limits<ld>::infinity();
  MinLineContainer<ld> minlc;
  MaxLineContainer<ld> maxlc;

 private:
  std::size_t count_ = 0;
  // 照会では状態を変えず、包絡が変わった追加後にだけ更新する。
  // 算術の代替指定と、処理系に合わせたキャッシュ選択を分ける。
#if (defined(__clang__) || defined(__GNUC__))
  static constexpr bool cache_points = true;
#else
  static constexpr bool cache_points = false;
#endif
  struct NoCache {};
  struct HintEntry {
    const Line<ld>* line = nullptr;
    const Line<ld>* previous = nullptr;
  };
  struct CachedHintEntry : HintEntry { Line<ld> cached{}; };
  struct AdjacentLines {
    const Line<ld>* before = nullptr;
    const Line<ld>* after = nullptr;
  };
  struct SearchHint {
    using Entry = std::conditional_t<cache_points, CachedHintEntry, HintEntry>;
    Entry positive, negative;
    // 0: 別区間、1/2: 全包絡、3: 共通区間、4/5: 境界が+1/-1の2本。
    unsigned small = 0;
    [[no_unique_address]] std::conditional_t<cache_points, AdjacentLines, NoCache> adjacent{};
    [[no_unique_address]] std::conditional_t<cache_points, Line<ld>, NoCache> after_cached{};
  } min_hint_, max_hint_;
  template <class Hull, class Hint>
#if (defined(__clang__) || defined(__GNUC__))
  [[gnu::always_inline]]
#endif
  static void refresh_hint(const Hull& hull, Hint& hint) {
    if constexpr (cache_points) {
      if (hull.empty()) { hint = {}; return; }
      hint.small = 0;
      hint.positive.previous = nullptr;
      hint.negative.line = nullptr;
      hint.negative.previous = nullptr;
      hint.adjacent = {};
    } else {
      hint = {};
      if (hull.empty()) return;
    }
    if (hull.size() <= 2) {
      // 2本なら先頭の右端は有限なので、+inf検索が必ず末尾を選ぶ。
      const auto first = hull.begin();
      hint.positive.line = &*first;
      hint.negative.line = hull.size() == 2 ? &*hull.lower_bound(lc_inf<ld>()) : &*first;
      if constexpr (cache_points) {
        // 1/2本では境界も更新する。3の境界には木の値を使う。
        { hint.positive.cached.k = (*hint.positive.line).k; hint.positive.cached.m = (*hint.positive.line).m; hint.positive.cached.p = (*hint.positive.line).p; }
        { hint.negative.cached.k = (*hint.negative.line).k; hint.negative.cached.m = (*hint.negative.line).m; }
      }
      hint.small = static_cast<unsigned>(hull.size());
#if defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
      if constexpr (cache_points && std::numeric_limits<ld>::radix == 2 &&
                    std::numeric_limits<ld>::is_iec559 &&
                    std::is_same_v<Hull, MinLineContainer<ld>>) {
        if (hint.small == 2) {
          if (std::abs(hint.positive.cached.p) == ld{1})
            hint.small = std::signbit(hint.positive.cached.p) ? 5 : 4;
        }
      }
#endif
      return;
    }
    const auto positive = hull.lower_bound(ld{0.5});
    hint.positive.line = &*positive;
    if constexpr (cache_points) { hint.positive.cached.k = (*positive).k; hint.positive.cached.m = (*positive).m; }
    hint.positive.previous = positive == hull.begin() ? nullptr : &*std::prev(positive);
    if (!hint.positive.previous || hint.positive.previous->p < ld{-0.5}) {
      hint.small = 3;
      if constexpr (cache_points) {
        if (positive != hull.begin()) {
          const auto prev = std::prev(positive);
          hint.adjacent.before = prev == hull.begin() ? nullptr : &*std::prev(prev);
          { hint.negative.cached.k = (*prev).k; hint.negative.cached.m = (*prev).m; }
        }
        const auto next = std::next(positive);
        hint.adjacent.after = next == hull.end() ? nullptr : &*next;
        if (hint.adjacent.after) { hint.after_cached.k = (*hint.adjacent.after).k; hint.after_cached.m = (*hint.adjacent.after).m; }
      }
      return;
    }
    const auto negative = hull.lower_bound(ld{-0.5});
    hint.negative.line = &*negative;
    if constexpr (cache_points) { hint.negative.cached.k = (*negative).k; hint.negative.cached.m = (*negative).m; }
    hint.negative.previous = negative == hull.begin() ? nullptr : &*std::prev(negative);
    if (positive == negative) hint.small = 3;
  }
  void refresh_hints() {
    refresh_hint(minlc, min_hint_);
    refresh_hint(maxlc, max_hint_);
  }
  template <bool use_shared = false, bool copy = false, class Hull, class Hint>
#if (defined(__clang__) || defined(__GNUC__))
  [[gnu::always_inline]]
#endif
  static std::conditional_t<copy, Line<ld>, const Line<ld>&> hinted_line(const Hull& hull, const Hint& hint, ld c) {
    if (hint.small == 3) {
      const auto& entry = hint.positive;
      if constexpr (cache_points) {
        // 中央の前後も、同じ境界を再利用して判定する。
        if (c <= entry.line->p) {
          if (entry.previous && c <= entry.previous->p) {
            if (!hint.adjacent.before || hint.adjacent.before->p < c) return hint.negative.cached;
          } else return entry.cached;
        } else if (hint.adjacent.after && c <= hint.adjacent.after->p) return hint.after_cached;
      } else if constexpr (use_shared) {
        if ((!entry.previous || entry.previous->p < c) && c <= entry.line->p) return *entry.line;
      }
      return *hull.lower_bound(c);
    }
    if (hint.small) {
      if constexpr (cache_points)
        return hint.positive.cached.p < c ? hint.negative.cached : hint.positive.cached;
      else return hint.positive.line->p < c ? *hint.negative.line : *hint.positive.line;
    }
    const auto& entry = c >= 0 ? hint.positive : hint.negative;
    if ((!entry.previous || entry.previous->p < c) && c <= entry.line->p) {
      if constexpr (cache_points) return entry.cached;
      else return *entry.line;
    }
    return *hull.lower_bound(c);
  }
  // 不要な追加を確保前に棄却し、照会キャッシュの更新要否を返す。
  template <Objective objective, class Hint>
#if (defined(__clang__) || defined(__GNUC__))
  [[gnu::always_inline]]
#endif
  static bool add_hull(LineContainer<ld, objective>& hull, ld k, ld m, const Hint& hint) {
    using Hull = LineContainer<ld, objective>;
    Line<ld> candidate{k, m, lc_inf<ld>()};
    auto z = hull.lower_bound(candidate);
    if (z != hull.end() && z->k == k) {
      const bool better = objective == MAXIMIZE ? m > z->m : m < z->m;
      if (!better) return false;
    }
    auto x = z;
    bool has_previous = z != hull.begin();
    Line<ld> previous{};
    if (z != hull.end()) {
      if (z->k == k) candidate.p = lc_inf<ld>();
      else Hull::intersection(candidate, *z);
    }
    if (has_previous) {
      --x;
      previous = *x;
      Hull::intersection(previous, candidate);
      if (z == hull.end() || candidate.p < z->p) {
        if (previous.p >= candidate.p) return false;
      }
    }
    // 共通区間の外だけが変わる場合は、既存の点・境界参照を保つ。
    bool refresh = true;
    ld lower = 0, upper = 0;
    if constexpr (cache_points) {
      if (hint.small == 3) {
        const auto* first = hint.adjacent.before;
        const auto* last = hint.adjacent.after ? hint.adjacent.after : hint.positive.line;
        lower = first ? first->k : (objective == MAXIMIZE ? -lc_inf<ld>() : lc_inf<ld>());
        upper = last->k;
        refresh = objective == MAXIMIZE ? lower <= k && k <= upper : upper <= k && k <= lower;
      }
    } else (void)hint;
    const auto mark_removed = [&](const Line<ld>& line) {
      if constexpr (cache_points) {
        if (!refresh) refresh = objective == MAXIMIZE ? lower <= line.k && line.k <= upper : upper <= line.k && line.k <= lower;
      }
    };
    auto y = hull.insert(z, candidate);
    bool removed_successor = false, changed = true;
    while (z != hull.end() && !Hull::boundary_less(*y, *z)) {
      mark_removed(*z);
      z = hull.erase(z);
      removed_successor = true;
      hull.insect(y, z);
    }
    if (has_previous) {
      x->p = previous.p;
      if (!Hull::boundary_less(*x, *y)) {
        mark_removed(*y);
        hull.insect(x, y = hull.erase(y));
        changed = removed_successor;
      }
    } else x = y;
    while ((y = x) != hull.begin() && !Hull::boundary_less(*--x, *y)) {
      mark_removed(*y);
      hull.insect(x, hull.erase(y));
      changed = true;
    }
    return changed && refresh;
  }
  static void check_coefficients(ld a, ld b) {
#ifndef SKIP_RUNTIME_CHECKS
    if (!std::isfinite(a) || !std::isfinite(b))
      throw std::invalid_argument("LineContainer2D: nonfinite coefficient");
#else
    (void)a; (void)b;
#endif
  }
#if (defined(__GNUC__) || defined(__clang__))
  [[gnu::noinline, gnu::cold]]
#endif
  static ld recover_line_dot(ld a, ld b, const Line<ld>* line) {
    // 巨大な有限積同士の相殺でもinf-infを返さない。
    const ld x = line->m, y = line->k;
    int ea = 0, eb = 0, ex = 0, ey = 0;
    std::frexp(a, &ea); std::frexp(b, &eb);
    std::frexp(x, &ex); std::frexp(y, &ey);
    const int e1 = std::max(ea, eb), e2 = std::max(ex, ey);
    const ld sa = std::ldexp(a, -e1), sb = std::ldexp(b, -e1);
    const ld sx = std::ldexp(x, -e2), sy = std::ldexp(y, -e2);
    return std::ldexp(sa * sx + sb * sy, e1 + e2);
  }
  static ll to_rounded_integer(ld value) {
    const ld limited = std::clamp(value, -INF, INF);
    const ll integral = static_cast<ll>(limited);
    const ld fraction = limited - ld(integral);
    return integral + (fraction >= 0.5L) - (fraction <= -0.5L);
  }
  template <bool ratio_is_finite = false>
  std::pair<ld, ld> select(ld a, ld b) const {
    if (a == 0) {
      if constexpr (!ratio_is_finite) check_coefficients(a, b);
      const auto it = b > 0 ? std::prev(maxlc.end()) : maxlc.begin();
      return {it->m, it->k};
    }
    if (b == 0) {
      if constexpr (!ratio_is_finite) check_coefficients(a, b);
      const auto& line = a > 0 ? *maxlc.lower_bound(ld{0}) : *minlc.lower_bound(ld{0});
      return {line.m, line.k};
    }
    return select_nonzero<ratio_is_finite>(a, b);
  }
  [[noreturn]] static void slope_error(ld a, ld b) {
    check_coefficients(a, b);
    throw std::overflow_error("LineContainer2D: query slope overflow");
  }
  template <bool ratio_is_finite>
  std::pair<ld, ld> select_nonzero(ld a, ld b) const {
    const ld c = b / a;
    if constexpr (!ratio_is_finite) {
#ifndef SKIP_RUNTIME_CHECKS
      // 有限aと有限b / aからbの有限性も分かる。bを重ねて検査しない。
      if (!std::isfinite(a) || !std::isfinite(c)) [[unlikely]] slope_error(a, b);
#else
      if (!std::isfinite(c)) [[unlikely]] slope_error(a, b);
#endif
    }
    const auto& line = [&]() -> const Line<ld>& {
      if constexpr (ratio_is_finite) return a > 0 ? *maxlc.lower_bound(c) : *minlc.lower_bound(c);
      else return a > 0 ? hinted_line<ratio_is_finite>(maxlc, max_hint_, c) : hinted_line<ratio_is_finite>(minlc, min_hint_, c);
    }();
    return {line.m, line.k};
  }
#if (defined(__GNUC__) || defined(__clang__))
  [[gnu::always_inline]]
#endif
  void check_empty() const {
#ifndef SKIP_RUNTIME_CHECKS
    if (empty()) throw std::out_of_range("LineContainer2D: empty query");
#endif
  }

#if defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
  [[gnu::noinline, gnu::cold]]
  static void check_small_ratio(ld a, ld b) {
    if (!std::isfinite(b / a)) slope_error(a, b);
  }
#endif
  template <bool proof, bool negative_a = false, bool optimized_float = false, class Hull, class Hint>
#if (defined(__clang__) || defined(__GNUC__))
  [[gnu::always_inline]]
#endif
  static ld nonzero_value(const Hull& hull, const Hint& hint, ld a, ld b) {
#if defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
    // 単位境界の比較では除算を省き、小さいaの商溢れだけ別途確認する。
    if constexpr (optimized_float && !proof && cache_points &&
                  std::numeric_limits<ld>::radix == 2 && std::numeric_limits<ld>::is_iec559 &&
                  std::is_same_v<Hull, MinLineContainer<ld>>) {
      if (hint.small >= 4) {
        if (std::abs(a) < ld{1}) [[unlikely]] check_small_ratio(a, b);
        const ld boundary = hint.small == 4 ? a : -a;
        const bool right = negative_a ? b < boundary : b > boundary;
        const auto l = right ? hint.negative.cached : hint.positive.cached;
        const ld result = a * l.m + b * l.k;
        if (std::isnan(result)) [[unlikely]] return recover_line_dot(a, b, &l);
        return result;
      }
    }
#endif
    const ld c = b / a;
#if defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
    const auto& l = hinted_line<(cache_points || proof), optimized_float && !proof>(hull,hint,c);
#else
    const auto& l = hinted_line<(cache_points || proof)>(hull, hint, c);
#endif
    const ld result = a * l.m + b * l.k;
    // 有限cと有限resultならa, bも有限。例外経路だけ個別検査する。
    if constexpr (proof) {
      if (std::isnan(result)) [[unlikely]] return recover_line_dot(a, b, &l);
    } else if constexpr (cache_points) {
      if (!(std::abs(c + result) <= std::numeric_limits<ld>::max())) [[unlikely]]
        return checked_result(a, b, c, result, &l);
    } else if (!std::isfinite(c + result)) [[unlikely]]
      return checked_result(a, b, c, result, &l);
    return result;
  }
  template <bool ratio_is_finite = false, bool optimized_float = false>
#if (defined(__GNUC__) || defined(__clang__))
  [[gnu::always_inline]]
#endif
  ld value(ld a, ld b) const {
    if (a > 0) {
      if (b == 0) [[unlikely]] {
        if constexpr (!ratio_is_finite) check_coefficients(a, b);
        return a * Xmax;
      }
      return nonzero_value<ratio_is_finite, false, optimized_float>(maxlc, max_hint_, a, b);
    }
    if (a < 0) {
      if (b == 0) [[unlikely]] {
        if constexpr (!ratio_is_finite) check_coefficients(a, b);
        return a * Xmin;
      }
      return nonzero_value<ratio_is_finite, true, optimized_float>(minlc, min_hint_, a, b);
    }
    if constexpr (!ratio_is_finite) check_coefficients(a, b);
    return b == 0 ? 0 : b * (b > 0 ? Ymax : Ymin);
  }
#if defined(__clang__)
  template <bool ratio_is_finite = false, bool optimized_float = false>
#if (defined(__GNUC__) || defined(__clang__))
  [[gnu::always_inline]]
#endif
  ld minimum_value(ld a, ld b) const {
    if (a > 0) {
      if (b == 0) [[unlikely]] {
        if constexpr (!ratio_is_finite) check_coefficients(a, b);
        return a * Xmin;
      }
      return nonzero_value<ratio_is_finite, false, optimized_float>(minlc, min_hint_, a, b);
    }
    if (a < 0) {
      if (b == 0) [[unlikely]] {
        if constexpr (!ratio_is_finite) check_coefficients(a, b);
        return a * Xmax;
      }
      return nonzero_value<ratio_is_finite, true, optimized_float>(maxlc, max_hint_, a, b);
    }
    if constexpr (!ratio_is_finite) check_coefficients(a, b);
    return b == 0 ? -ld{0} : b * (b > 0 ? Ymin : Ymax);
  }
#endif
#if (defined(__GNUC__) || defined(__clang__))
  [[gnu::noinline, gnu::cold]]
#endif
  static ld checked_result(ld a, ld b, ld c, ld result, const Line<ld>* line) {
#ifndef SKIP_RUNTIME_CHECKS
    if (!std::isfinite(a) || !std::isfinite(c)) slope_error(a, b);
#else
    if (!std::isfinite(c)) slope_error(a, b);
#endif
    return std::isnan(result) ? recover_line_dot(a, b, line) : result;
  }
  template <bool minimum>
  ld integer_value(ll a, ll b) const {
#ifndef SKIP_RUNTIME_CHECKS
    if (empty()) throw std::out_of_range("LineContainer2D: empty query");
#endif
    // int64係数の変換と非零分母との比は必ず有限。重複する浮動検査を省く。
    if constexpr (minimum) return -value<true>(-ld(a), -ld(b));
    else return value<true>(ld(a), ld(b));
  }

#if defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
  [[noreturn, gnu::noinline, gnu::cold]]
  static void count_error() { throw std::length_error("LineContainer2D: point count overflow"); }
#endif
 public:
  LineContainer2D() = default;
  LineContainer2D(const LineContainer2D& other)
      : Xmax(other.Xmax), Xmin(other.Xmin), Ymax(other.Ymax), Ymin(other.Ymin),
        minlc(other.minlc), maxlc(other.maxlc), count_(other.count_) { refresh_hints(); }
  LineContainer2D& operator = (const LineContainer2D& other) {
    if (this != &other) { LineContainer2D copy(other); swap(copy); }
    return *this;
  }
  LineContainer2D(LineContainer2D&& other) noexcept
      : Xmax(other.Xmax), Xmin(other.Xmin), Ymax(other.Ymax), Ymin(other.Ymin),
        minlc(std::move(other.minlc)), maxlc(std::move(other.maxlc)), count_(other.count_),
        min_hint_(other.min_hint_), max_hint_(other.max_hint_) {
    other.clear();
  }
  LineContainer2D& operator = (LineContainer2D&& other) noexcept {
    if (this != &other) {
      minlc = std::move(other.minlc); maxlc = std::move(other.maxlc);
      Xmax = other.Xmax; Xmin = other.Xmin; Ymax = other.Ymax; Ymin = other.Ymin;
      count_ = other.count_;
      min_hint_ = other.min_hint_; max_hint_ = other.max_hint_;
      other.clear();
    }
    return *this;
  }
#if (defined(__clang__) || defined(__GNUC__))
  [[gnu::always_inline]]
#endif
  void add(ld x, ld y) {
    check_coefficients(x, y);
#if defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
    if (count_ == std::numeric_limits<std::size_t>::max()) [[unlikely]] count_error();
#else
    if (count_ == std::numeric_limits<std::size_t>::max())
      throw std::length_error("LineContainer2D: point count overflow");
#endif
    bool refresh_min, refresh_max;
    try {
      refresh_min = add_hull(minlc, y, x, min_hint_);
      refresh_max = add_hull(maxlc, y, x, max_hint_);
    }
    catch (...) { clear(); throw; }
    if (refresh_min) refresh_hint(minlc, min_hint_);
    if (refresh_max) refresh_hint(maxlc, max_hint_);
    Xmax = std::max(Xmax, x); Xmin = std::min(Xmin, x);
    Ymax = std::max(Ymax, y); Ymin = std::min(Ymin, y);
    ++count_;
  }
  template <class A, class B>
    requires (std::is_integral_v<A> && std::is_integral_v<B> && std::numeric_limits<A>::digits <= 63 &&
              std::numeric_limits<B>::digits <= 63)
  ld max_ld(A a, B b) const { return integer_value<false>(a, b); }
  template <class A, class B>
    requires (std::is_integral_v<A> && std::is_integral_v<B> && std::numeric_limits<A>::digits <= 63 &&
              std::numeric_limits<B>::digits <= 63)
  ld min_ld(A a, B b) const { return integer_value<true>(a, b); }
#if (defined(__clang__) || defined(__GNUC__))
  [[gnu::always_inline]]
#endif
  ld max_ld(ld a, ld b) const {
    check_empty();
    return value<false, true>(a, b);
  }
#if (defined(__clang__) || defined(__GNUC__))
  [[gnu::always_inline]]
#endif
  ld min_ld(ld a, ld b) const {
#if defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
    check_empty(); return minimum_value<false, true>(a,b);
#else
    return -max_ld(-a,-b);
#endif
  }
  ll max_ll(ll a, ll b) const { return static_cast<ll>(std::clamp(integer_value<false>(a, b), -INF, INF)); }
  ll min_ll(ll a, ll b) const { return static_cast<ll>(std::clamp(integer_value<true>(a, b), -INF, INF)); }
  ll max_rounded_ll(ll a, ll b) const { return to_rounded_integer(integer_value<false>(a, b)); }
  ll min_rounded_ll(ll a, ll b) const { return to_rounded_integer(integer_value<true>(a, b)); }
  Result max_with_point(ld a, ld b) const {
    check_empty();
    const auto [x, y] = select(a, b);
    ld result = a * x + b * y;
    if (std::isnan(result)) [[unlikely]] {
      const Line<ld> line{y, x, 0};
      result = recover_line_dot(a, b, &line);
    }
    return {result, x, y};
  }
  Result min_with_point(ld a, ld b) const {
    const auto result = max_with_point(-a, -b);
    return {-result.value, result.x, result.y};
  }
  std::pair<ld, ld> argmax(ld a, ld b) const { check_empty(); return select(a, b); }
  std::pair<ld, ld> argmin(ld a, ld b) const { return argmax(-a, -b); }
#if (defined(__clang__) || defined(__GNUC__))
  [[gnu::always_inline]]
#endif
  std::optional<ld> max_optional(ld a, ld b) const {
    if (empty()) { check_coefficients(a, b); return std::nullopt; }
    return value(a, b);
  }
#if (defined(__clang__) || defined(__GNUC__))
  [[gnu::always_inline]]
#endif
  std::optional<ld> min_optional(ld a, ld b) const {
    return max_optional(-a,-b).transform([](ld v){return -v;});
  }
  std::optional<std::pair<ld, ld>> argmax_optional(ld a, ld b) const {
    if (empty()) { check_coefficients(a, b); return std::nullopt; }
    return select(a, b);
  }
  std::optional<std::pair<ld, ld>> argmin_optional(ld a, ld b) const { return argmax_optional(-a, -b); }
  void swap(LineContainer2D& other) noexcept {
    minlc.swap(other.minlc); maxlc.swap(other.maxlc);
    std::swap(Xmax, other.Xmax); std::swap(Xmin, other.Xmin);
    std::swap(Ymax, other.Ymax); std::swap(Ymin, other.Ymin);
    std::swap(count_, other.count_);
    std::swap(min_hint_, other.min_hint_); std::swap(max_hint_, other.max_hint_);
  }
  std::size_t size() const noexcept { return count_; }
  bool empty() const noexcept { return count_ == 0; }
  void clear() noexcept {
    minlc.clear(); maxlc.clear(); count_ = 0;
    min_hint_ = {}; max_hint_ = {};
    Xmax = Ymax = -std::numeric_limits<ld>::infinity();
    Xmin = Ymin = std::numeric_limits<ld>::infinity();
  }
};
#endif
