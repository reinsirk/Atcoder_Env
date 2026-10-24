// SPDX-License-Identifier: CC0-1.0
// 検査用リンク引数: -Wl,--wrap=malloc,--wrap=realloc
// 総当たり・多倍長演算で閉区間、全整数幅、所有権、入力検査を照合する。
#ifndef LI_CHAO_SOURCE
#define LI_CHAO_SOURCE "../../../DataStructure/dynamic-li-chao-tree.cpp"
#endif
#include LI_CHAO_SOURCE
#include <algorithm>
#include <array>
#include <bit>
#include <boost/multiprecision/cpp_int.hpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <new>
#include <random>
#include <source_location>
#include <type_traits>
#include <utility>
#include <vector>

static int remaining_allocations = -1;
static std::size_t allocations = 0, checks = 0, failure_points = 0;
extern "C" void* __real_malloc(std::size_t);
extern "C" void* __real_realloc(void*, std::size_t);
static bool deny_allocation() {
  ++allocations;
  if (remaining_allocations == 0) return true;
  if (remaining_allocations > 0) --remaining_allocations;
  return false;
}
extern "C" void* __wrap_malloc(std::size_t bytes) {
  return deny_allocation() ? nullptr : __real_malloc(bytes);
}
extern "C" void* __wrap_realloc(void* pointer, std::size_t bytes) {
  return deny_allocation() ? nullptr : __real_realloc(pointer, bytes);
}

[[gnu::noinline]] void* operator new(std::size_t n) {
  ++allocations;
  if (remaining_allocations == 0) throw std::bad_alloc();
  if (remaining_allocations > 0) --remaining_allocations;
  if (void* p = std::malloc(n ? n : 1)) return p;
  throw std::bad_alloc();
}
[[gnu::noinline]] void* operator new[](std::size_t n) { return ::operator new(n); }
[[gnu::noinline]] static void release(void* p) noexcept { std::free(p); }
void operator delete(void* p) noexcept { release(p); }
void operator delete[](void* p) noexcept { release(p); }
void operator delete(void* p, std::size_t) noexcept { release(p); }
void operator delete[](void* p, std::size_t) noexcept { release(p); }

static void require(bool value,
                    std::source_location loc = std::source_location::current()) {
  ++checks;
  if (!value) {
    std::fprintf(stderr, "failed line %u\n", loc.line());
    std::abort();
  }
}
using I = std::int64_t;
using Big = boost::multiprecision::cpp_int;
static Big evaluate(I a, I b, I x) { return Big(a) * x + b; }

template<class T, T Inf = std::numeric_limits<T>::max()>
struct Reference {
  using Tree = DynamicLiChaoTree<T, Inf>;
  struct Record { T a, b, l, r; };
  T left, right;
  Tree tree;
  std::vector<Record> lines;
  explicit Reference(T l, T r) : left(l), right(r), tree(l, r) {}
  void line(T a, T b, bool alias = false) {
    if (alias) tree.update(a, b);
    else tree.add_line(a, b);
    lines.push_back({a, b, left, right});
  }
  void segment(T a, T b, T l, T r, bool alias = false) {
    if (alias) tree.update_line_segment(a, b, l, r);
    else tree.add_segment(a, b, l, r);
    if (l <= r) lines.push_back({a, b, l, r});
  }
  void verify(T x) const {
    Big expected = Inf;
    for (const Record& line : lines)
      if (line.l <= x && x <= line.r)
        expected = std::min(expected, evaluate(line.a, line.b, x));
    const auto stored = tree.stored_nodes();
    const auto before_allocations = allocations;
    remaining_allocations = 0;
    const T answer = tree.query(x);
    const auto actual = tree.query_line(x);
    remaining_allocations = -1;
    require(allocations == before_allocations);
    require(Big(answer) == expected);
    require(bool(actual) == (expected != Big(Inf)));
    if (actual) {
      require(evaluate(actual->slope, actual->intercept, x) == expected);
      bool belonged = false;
      for (const Record& line : lines)
        belonged |= line.l <= x && x <= line.r &&
                    line.a == actual->slope && line.b == actual->intercept;
      require(belonged);
    }
    require(tree.stored_nodes() == stored);
    require(tree.domain_min() == left && tree.domain_max() == right);
  }
  void clear() { tree.clear(); lines.clear(); }
};

template<class T>
static void random_case(I lo, I hi, I a_limit, I b_limit) {
  for (unsigned seed = 0; seed != 8; ++seed) {
    std::mt19937_64 rng(7821 + seed);
    Reference<T> state(static_cast<T>(lo), static_cast<T>(hi));
    auto point = [&] { return static_cast<T>(lo + I(rng() % std::uint64_t(hi - lo + 1))); };
    auto value = [&](I limit) { return static_cast<T>(I(rng() % std::uint64_t(2 * limit + 1)) - limit); };
    require(state.tree.empty());
    state.verify(state.left); state.verify(state.right);
    state.tree.reserve(1000);
    require(state.tree.empty() && state.tree.stored_nodes() == 0);
    for (int step = 0; step != 120; ++step) {
      const T a = value(a_limit), b = value(b_limit);
      T l = point(), r = point();
      if (l > r) std::swap(l, r);
      if (step % 3 == 0) state.line(a, b, step % 2);
      else state.segment(a, b, l, r, step % 2);
      require(!state.tree.empty());
      for (T x : std::array<T, 6>{state.left, state.right, T(0), l, r, point()})
        state.verify(x);
      if (step % 27 == 0) {
        // 値コピーを独立に更新し、元の木が変わらないことも確認する。
        auto clone = state;
        clone.line(T(0), static_cast<T>(-b_limit - 1));
        clone.verify(l); clone.verify(r); state.verify(l); state.verify(r);
        Reference<T> assigned(T(-1), T(1));
        assigned.tree = state.tree; assigned.left = state.left;
        assigned.right = state.right; assigned.lines = state.lines;
        assigned.verify(l); assigned.verify(r);
        auto moved = std::move(clone);
        moved.verify(l); moved.verify(r);
        Reference<T> move_assigned(T(-1), T(1));
        move_assigned = std::move(moved);
        move_assigned.verify(l); move_assigned.verify(r);
        clone.tree.clear();
        require(clone.tree.empty());
        clone.tree.add_line(T(0), T(3));
        require(clone.tree.query(T(0)) == T(3));
        auto* alias = &assigned.tree;
        assigned.tree = *alias;
        assigned.verify(l);
      }
    }
    state.clear();
    require(state.tree.empty() && state.tree.stored_nodes() == 0);
    state.verify(state.left); state.verify(state.right);
    const auto before = allocations;
    state.line(T(0), T(-7));
    // clearは既存のノード容量を利用できる。
    require(allocations == before || allocations == before + 1); // 参照の記録領域のみ許す。
    state.verify(state.left); state.verify(state.right);
  }
}

template<class T>
static void boundary_case() {
  constexpr T lo = std::numeric_limits<T>::min();
  constexpr T hi = std::numeric_limits<T>::max();
  Reference<T> state(lo, hi);
  state.verify(lo); state.verify(hi);
  state.segment(T(1), T(0), lo, static_cast<T>(hi - 1));
  state.segment(T(-1), T(-1), static_cast<T>(lo + 1), hi);
  state.segment(lo, lo, T(-1), T(-1)); // 中間積 -MIN が表現範囲外でも相殺後は0。
  state.segment(hi, lo, T(2), T(2));  // 2*MAX+MIN = MAX-1。
  state.segment(lo, static_cast<T>(hi - 1), T(0), T(1));
  for (T x : std::array<T, 10>{lo, static_cast<T>(lo + 1), static_cast<T>(lo + 2),
                              T(-2), T(-1), T(0), T(1), T(2),
                              static_cast<T>(hi - 1), hi}) state.verify(x);
  const auto nodes = state.tree.stored_nodes();
  state.segment(hi, hi, T(1), T(0)); // 空線分は値・端点検査前に無操作。
  require(state.tree.stored_nodes() == nodes);
  state.line(T(0), static_cast<T>(hi - 1));
  state.verify(lo); state.verify(hi);
  auto clone = state;
  clone.verify(lo); clone.verify(hi);
  state.clear(); state.verify(lo); state.verify(hi);
  state.segment(T(0), lo, hi, hi);
  state.verify(lo); state.verify(hi);

  Reference<T> singleton(lo, lo);
  singleton.line(T(0), lo);
  singleton.verify(lo);
  singleton.clear();
  singleton.segment(T(1), T(0), lo, lo);
  singleton.verify(lo);

  // 切片INFでも非零の傾きで値域内になる直線・線分は有効。
  Reference<T> inf_intercept(T(1), T(2));
  inf_intercept.line(T(-1), hi);
  inf_intercept.verify(T(1)); inf_intercept.verify(T(2));
  inf_intercept.segment(T(-2), hi, T(1), T(1));
  inf_intercept.verify(T(1)); inf_intercept.verify(T(2));

  // 公開Lineは中間積が大きい場合も数学的な最終値と一致する。
  using Line = typename Reference<T>::Tree::Line;
  const Line a(lo, lo), b(hi, lo), constant(T(0), T(0));
  require(a.get(T(-1)) == T(0));
  require(b.get(T(2)) == static_cast<T>(hi - 1));
  require(!a.over(constant, T(-1)) && !constant.over(a, T(-1)));
}

static void narrow_line_exhaustive() {
  using T = std::int8_t;
  using Tree = DynamicLiChaoTree<T, 127>;
  for (int a = -128; a <= 127; ++a)
    for (int b = -128; b <= 127; ++b)
      for (int x : {-128, -17, -1, 0, 1, 19, 127}) {
        const int expected = a * x + b;
        if (-128 <= expected && expected <= 127) {
          const typename Tree::Line line(static_cast<T>(a), static_cast<T>(b));
          require(line.get(static_cast<T>(x)) == expected);
        }
      }
}

#ifndef SKIP_RUNTIME_CHECKS
template<class Exception, class F> static void throws(F&& operation) {
  bool caught = false;
  try { operation(); } catch (const Exception&) { caught = true; }
  require(caught);
}
static void checked_inputs() {
  using Tree = DynamicLiChaoTree<I, std::numeric_limits<I>::max()>;
  constexpr I lo = std::numeric_limits<I>::min(), hi = std::numeric_limits<I>::max();
  throws<std::invalid_argument>([] { Tree tree(1, -1); });
  Tree tree(-8, 9);
  throws<std::out_of_range>([&] { (void)tree.query(-9); });
  throws<std::out_of_range>([&] { (void)tree.query_line(10); });
  throws<std::out_of_range>([&] { tree.add_segment(0, 1, -9, 0); });
  throws<std::out_of_range>([&] { tree.add_segment(0, 1, 0, 10); });
  throws<std::overflow_error>([&] { tree.add_line(0, hi); });
  throws<std::overflow_error>([&] { tree.add_line(hi, 0); });
  throws<std::overflow_error>([&] { tree.add_segment(lo, 0, -1, -1); });
  require(tree.empty() && tree.stored_nodes() == 0);
  // 独立な多倍長演算で、全ビット幅の積と相殺を含む入力検査を照合。
  std::mt19937_64 rng(59931);
  for (int rep = 0; rep != 12000; ++rep) {
    const I a = std::bit_cast<I>(rng()), b = std::bit_cast<I>(rng());
    const I x = rep % 2 ? I(rng() % 7) - 3 : std::bit_cast<I>(rng());
    const Big expected = evaluate(a, b, x);
    const bool valid = Big(lo) <= expected && expected < Big(hi);
    Tree point(x, x);
    bool caught = false;
    try { point.add_line(a, b); } catch (const std::overflow_error&) { caught = true; }
    require(caught != valid);
    if (valid) require(Big(point.query(x)) == expected);
    else require(point.empty());
  }
  // 小さいINFも空の印として区別する。
  DynamicLiChaoTree<std::int16_t, 17> limited(-4, 4);
  limited.add_line(0, 16); require(limited.query(2) == 16);
  throws<std::overflow_error>([&] { limited.add_line(0, 17); });
  throws<std::overflow_error>([&] { limited.add_line(4, 1); });
}
template<class T> static void check_narrow_validator() {
  using U = std::make_unsigned_t<T>;
  constexpr T lo = std::numeric_limits<T>::min(), hi = std::numeric_limits<T>::max();
  std::mt19937_64 rng(39172);
  for (int rep = 0; rep != 4096; ++rep) {
    const T a = std::bit_cast<T>(static_cast<U>(rng()));
    const T b = std::bit_cast<T>(static_cast<U>(rng()));
    const T x = rep % 2 ? static_cast<T>(I(rng() % 7) - 3)
                       : std::bit_cast<T>(static_cast<U>(rng()));
    const Big expected = evaluate(a, b, x);
    const bool valid = Big(lo) <= expected && expected < Big(hi);
    DynamicLiChaoTree<T, hi> tree(x, x);
    bool failed = false;
    try { tree.add_line(a, b); } catch (const std::overflow_error&) { failed = true; }
    require(failed != valid);
    if (valid) require(Big(tree.query(x)) == expected);
    else require(tree.empty());
  }
}
#endif

#ifndef LI_CHAO_SKIP_COPY_FAILURE_TEST
static void ownership_failure() {
  using Tree = DynamicLiChaoTree<I, std::numeric_limits<I>::max()>;
  Tree source(-1000, 1000);
  for (I x = -100; x <= 100; x += 2) source.add_segment(0, x, x, x);
  Tree destination(-5, 7);
  destination.add_line(1, 3);
  const auto old_nodes = destination.stored_nodes();
  bool failed = false;
  remaining_allocations = 0;
  try { destination = source; } catch (const std::bad_alloc&) { failed = true; }
  remaining_allocations = -1;
  require(failed); ++failure_points;
  require(destination.domain_min() == -5 && destination.domain_max() == 7);
  require(destination.stored_nodes() == old_nodes);
  for (I x = -5; x <= 7; ++x) require(destination.query(x) == x + 3);
  destination = source;
  require(destination.domain_min() == -1000 && destination.domain_max() == 1000);
  require(destination.stored_nodes() == source.stored_nodes());
  for (I x = -105; x <= 105; ++x) require(destination.query(x) == source.query(x));
  // 十分な容量があればコピー代入に追加確保は不要。
  remaining_allocations = 0;
  destination = source;
  remaining_allocations = -1;
  destination.clear(); destination.add_line(0, -999);
  require(destination.query(0) == -999 && source.query(0) == 0);
}
#endif

int main() {
  random_case<std::int8_t>(-11, 12, 3, 40);
  random_case<std::int16_t>(-109, 128, 17, 500);
  random_case<std::int32_t>(-200003, 200011, 1024, 100000);
  random_case<std::int64_t>(-1000000000, 1000000000, 1000000000, 1000000000);
  boundary_case<std::int8_t>(); boundary_case<std::int16_t>();
  boundary_case<std::int32_t>(); boundary_case<std::int64_t>();
  narrow_line_exhaustive();
#ifndef SKIP_RUNTIME_CHECKS
  checked_inputs();
  check_narrow_validator<std::int8_t>();
  check_narrow_validator<std::int16_t>();
  check_narrow_validator<std::int32_t>();
#endif
  // 容量・添字の上限はSKIP_RUNTIME_CHECKSでも検査する。
  bool length_failed = false;
  DynamicLiChaoTree<I, std::numeric_limits<I>::max()> reserve_test(0, 0);
  try { reserve_test.reserve(std::numeric_limits<std::size_t>::max()); }
  catch (const std::length_error&) { length_failed = true; }
  require(length_failed && reserve_test.empty());
#ifndef LI_CHAO_SKIP_COPY_FAILURE_TEST
  ownership_failure();
#endif
  std::printf("{\"checks\":%zu,\"allocation_failure_points\":%zu,\"status\":\"pass\"}\n",
              checks, failure_points);
}
