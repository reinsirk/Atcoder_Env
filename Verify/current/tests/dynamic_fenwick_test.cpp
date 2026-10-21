// SPDX-License-Identifier: CC0-1.0
// 実行例: c++ -std=c++23 -O2 Verify/current/tests/dynamic_fenwick_test.cpp -o /tmp/fenwick_test
// 検査省略は-DSKIP_RUNTIME_CHECKS、別実装は-DFENWICK_SOURCE='"絶対パス"'で指定する。
#ifndef FENWICK_SOURCE
#define FENWICK_SOURCE "../../../DataStructure/dynamic-binary-indexed-tree.cpp"
#endif
#include FENWICK_SOURCE
#include FENWICK_SOURCE
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <random>
#include <set>
#include <source_location>
#include <type_traits>
#include <utility>
#include <vector>

using I = std::int64_t;
using U = std::uint64_t;
static U checks = 0;
static std::mt19937_64 rng(UINT64_C(0x28190353af));
static void require(bool ok, std::source_location where = std::source_location::current()) { ++checks; if (!ok) { std::fprintf(stderr, "failed check %llu at line %u\n", static_cast<unsigned long long>(checks), where.line()); std::abort(); } }

// 内部の表現を参照せず、更新が到達する1始まりの添字だけを数える。
static void record_nodes(std::set<U>& nodes, U n, U k) {
    for (U i = k + 1; i <= n;) {
        nodes.insert(i);
        const U step = i & (U{0} - i);
        if (step > n - i) break;
        i += step;
    }
}

template<class S>
struct Oracle {
    U n;
    DynamicFenwickTree<S, I> bit;
    std::map<U, I> values;
    std::set<U> nodes;
    explicit Oracle(U length) : n(length), bit(static_cast<S>(length)) {}
    void add(U k, I v) {
        bit.add(static_cast<S>(k), v); values[k] += v; record_nodes(nodes, n, k);
        require(bit.stored_nodes() == nodes.size());
    }
    void set(U k, I v) {
        bit.set(static_cast<S>(k), v); values[k] = v; record_nodes(nodes, n, k);
        require(bit.stored_nodes() == nodes.size());
    }
    I sum(U l, U r) const {
        I result = 0;
        for (auto [k, v] : values) if (l <= k && k < r) result += v;
        return result;
    }
    void query(U l, U r) {
        const auto before = bit.stored_nodes();
        require(bit.sum(static_cast<S>(l), static_cast<S>(r)) == sum(l, r));
        require(bit.sum(static_cast<S>(r)) == sum(0, r));
        require(bit.sum_all() == sum(0, n));
        require(bit.stored_nodes() == before);
    }
    void point(U k) {
        const auto before = bit.stored_nodes();
        const auto it = values.find(k); const I expected = it == values.end() ? 0 : it->second;
        require(bit.get(static_cast<S>(k)) == expected);
        require(bit[static_cast<S>(k)] == expected);
        require(bit.stored_nodes() == before);
    }
    U bound(I w, bool upper) const {
        if (upper ? w < 0 : w <= 0) return 0;
        I prefix = 0;
        for (auto [k, v] : values) {
            prefix += v;
            if (upper ? prefix > w : prefix >= w) return k;
        }
        return n;
    }
    void search(I w) {
        const auto before = bit.stored_nodes();
        require(static_cast<U>(bit.lower_bound(w)) == bound(w, false));
        require(static_cast<U>(bit.upper_bound(w)) == bound(w, true));
        require(bit.stored_nodes() == before);
    }
};

static void small_and_dense() {
    std::vector<int> lengths;
    for (int n = 0; n <= 135; ++n) lengths.push_back(n);
    for (int n : {255, 256, 257, 1023, 1024, 4095, 4096, 8191, 8192, 65536}) lengths.push_back(n);
    for (int n : lengths) {
        Oracle<int> o(n);
        require(o.bit.size() == n && o.bit.empty() == (n == 0));
        o.query(0, n); require(o.bit.stored_nodes() == 0);
        o.bit.reserve(0); o.bit.reserve(std::min(130, n)); require(o.bit.stored_nodes() == 0);
        for (int t = 0; t < 800; ++t) {
            if (n) {
                const U k = rng() % n;
                const I value = static_cast<I>(rng() % 31) - 15;
                if (t % 3) o.add(k, value); else o.set(k, value);
                o.point(k);
            }
            U l = rng() % (n + 1), r = rng() % (n + 1); if (l > r) std::swap(l, r);
            o.query(l, r);
        }
        o.bit.clear(); o.values.clear(); o.nodes.clear();
        require(o.bit.size() == n && o.bit.stored_nodes() == 0); o.query(0, n);
        // 零の更新もセルを生成する。全0状態の探索と空区間を確認する。
        if (n) { o.add(n / 2, 0); o.query(0, n); o.search(0); o.search(1); }
        for (int t = 0; n && t < 100; ++t) o.add(rng() % n, static_cast<I>(rng() % 8));
        const I total = o.sum(0, n);
        for (I w = -2; w <= total + 2; ++w) o.search(w);
        o.bit.reset(n); require(o.bit.size() == n && o.bit.sum_all() == 0 && o.bit.stored_nodes() == 0);
    }
    // 密な保持形式へ変わる前後のセル数と、clear/reset後の再利用を確認する。
    Oracle<int> dense(65536);
    for (U i = 0; i < dense.n; i += 2) dense.add(i, static_cast<I>(i % 17));
    for (int t = 0; t < 120; ++t) {
        U l = rng() % dense.n, r = rng() % (dense.n + 1); if (l > r) std::swap(l, r);
        dense.query(l, r); dense.point(rng() % dense.n);
    }
    dense.bit.clear(); require(dense.bit.stored_nodes() == 0 && dense.bit.sum_all() == 0);
    dense.bit.add(65535, 7); require(dense.bit.get(65535) == 7 && dense.bit.sum_all() == 7);
}

template<class S> static void sparse_boundary(U n) {
    Oracle<S> o(n); require(static_cast<U>(o.bit.size()) == n);
    for (U k : std::array<U, 6>{0, 1, n / 2, n / 2 - 1, n - 2, n - 1}) o.add(k, 3);
    for (int t = 0; t < 160; ++t) {
        const U k = rng() % n;
        if (t % 3) o.add(k, static_cast<I>(rng() % 7)); else o.set(k, static_cast<I>(rng() % 7));
        U l = rng() % n, r = rng() % n; if (l > r) std::swap(l, r);
        o.query(l, r); o.query(l, n); o.point(k);
    }
    o.query(0, n); o.query(n, n); o.point(n - 1);
    const I total = o.sum(0, n);
    for (I w = -1; w <= total + 1; ++w) o.search(w);
    auto copied = o.bit;
    require(copied.sum_all() == total && copied.stored_nodes() == o.nodes.size());
    o.bit.add(0, 1); require(copied.sum_all() == total);
    auto moved = std::move(copied);
    require(moved.sum_all() == total && copied.size() == S{0} && copied.stored_nodes() == 0 && copied.sum_all() == 0);
    copied.reset(static_cast<S>(n)); copied = moved; require(copied.sum_all() == total);
    copied = std::move(moved); require(copied.sum_all() == total && moved.empty() && moved.stored_nodes() == 0);
    moved.reset(static_cast<S>(n)); moved.add(static_cast<S>(n - 1), 11); require(moved.sum_all() == 11);
    copied.clear(); require(copied.size() == static_cast<S>(n) && copied.sum_all() == 0 && copied.stored_nodes() == 0);
    copied.reset(S{0}); require(copied.empty() && copied.sum(S{0}, S{0}) == 0);
}

// noexceptでない構築・代入でも使える加法型。整数専用処理へ入らないことを確認する。
struct Number {
    I value;
    Number() noexcept(false) : value(0) {}
    explicit Number(I v) noexcept : value(v) {}
    Number(const Number&) = default;
    Number& operator=(const Number& v) noexcept(false) { value = v.value; return *this; }
    Number& operator+=(const Number& v) { value += v.value; return *this; }
    Number& operator-=(const Number& v) { value -= v.value; return *this; }
    friend Number operator-(Number a, const Number& b) { return a -= b; }
    friend bool operator<(Number a, Number b) { return a.value < b.value; }
    friend bool operator<=(Number a, Number b) { return a.value <= b.value; }
};

static void generic_values() {
    for (int n : {0, 1, 63, 64, 65, 257, 4096, 1000001}) {
        DynamicFenwickTree<int, double> floating(n);
        DynamicFenwickTree<int, Number> object(n);
        std::map<int, I> values;
        for (int t = 0; n && t < 240; ++t) {
            const int k = static_cast<int>(rng() % n); const I v = static_cast<I>(rng() % 7);
            if (t % 3) { floating.add(k, static_cast<double>(v) / 8); object.add(k, Number(v)); values[k] += v; }
            else { floating.set(k, static_cast<double>(v) / 8); object.set(k, Number(v)); values[k] = v; }
            int l = static_cast<int>(rng() % (n + 1)), r = static_cast<int>(rng() % (n + 1)); if (l > r) std::swap(l, r);
            I sum = 0, prefix = 0, total = 0;
            for (auto [i, x] : values) { total += x; if (l <= i && i < r) sum += x; if (i < r) prefix += x; }
            const auto nf = floating.stored_nodes(), no = object.stored_nodes();
            require(floating.sum(l, r) == static_cast<double>(sum) / 8 && object.sum(l, r).value == sum);
            require(floating.sum(r) == static_cast<double>(prefix) / 8 && object.sum(r).value == prefix);
            require(floating.sum_all() == static_cast<double>(total) / 8 && object.sum_all().value == total);
            require(floating.get(k) == static_cast<double>(values[k]) / 8 && object.get(k).value == values[k]);
            require(floating.stored_nodes() == nf && object.stored_nodes() == no);
        }
        I total = 0; for (auto [i, v] : values) total += v;
        for (I w = -1; w <= total + 1; ++w) {
            int lower = 0, upper = 0; I prefix = 0;
            if (w > 0) { lower = n; for (auto [i, v] : values) { prefix += v; if (prefix >= w) { lower = i; break; } } }
            prefix = 0;
            if (w >= 0) { upper = n; for (auto [i, v] : values) { prefix += v; if (prefix > w) { upper = i; break; } } }
            require(floating.lower_bound(static_cast<double>(w) / 8) == lower && object.lower_bound(Number(w)) == lower);
            require(floating.upper_bound(static_cast<double>(w) / 8) == upper && object.upper_bound(Number(w)) == upper);
        }
        auto f = floating; auto c = object; require(f.sum_all() == floating.sum_all() && c.sum_all().value == object.sum_all().value);
        f.clear(); c.clear(); require(f.sum_all() == 0 && c.sum_all().value == 0 && f.stored_nodes() == 0 && c.stored_nodes() == 0);
        f.reset(n); c.reset(n); if (n) { f.add(n - 1, 0.5); c.add(n - 1, Number(4)); require(f.sum_all() == 0.5 && c.sum_all().value == 4); }
    }
}

static void ownership() {
    for (int n : {0, 1, 64, 65, 4096, 65536, 1000001}) {
        DynamicFenwickTree<int, I> source(n);
        if (n) for (int t = 0; t < 1000; ++t) source.add(static_cast<int>(rng() % n), t % 11);
        const I total = source.sum_all(); const auto count = source.stored_nodes();
        DynamicFenwickTree<int, I> target(1000001); target.add(1000000, 61);
        target = source; require(target.size() == n && target.sum_all() == total && target.stored_nodes() == count);
        for (int t = 0; n && t < 30; ++t) {
            const int k = static_cast<int>(rng() % n);
            require(target.get(k) == source.get(k)); require(target.sum(k, n) == source.sum(k, n));
        }
        if (n) { target.add(n - 1, 3); require(source.sum_all() == total); target.add(n - 1, -3); }
        auto* alias = &target; target = *alias; require(target.sum_all() == total);
        target = std::move(*alias); require(target.size() == n && target.sum_all() == total);
        const auto target_nodes = target.stored_nodes();
        DynamicFenwickTree<int, I> destination(3); destination.add(0, 90);
        destination = std::move(target);
        require(destination.size() == n && destination.sum_all() == total && destination.stored_nodes() == target_nodes);
        require(target.empty() && target.sum_all() == 0 && target.stored_nodes() == 0);
        target.reset(65); target.add(64, 7); require(target.sum_all() == 7);
        destination.reset(1); destination.add(0, 5); require(destination.sum_all() == 5 && source.sum_all() == total);
    }
}

template<class T> static void unsigned_values() {
    static_assert(std::is_unsigned_v<T>);
    for (int n : {63, 64, 65, 257, 8192, 1000001}) {
        DynamicFenwickTree<int, T> bit(n); std::map<int, T> reference; std::set<U> nodes;
        for (int t = 0; t < 700; ++t) {
            const int k = static_cast<int>(rng() % n); const T value = static_cast<T>(rng());
            if (t % 3) { bit.add(k, value); reference[k] = static_cast<T>(reference[k] + value); }
            else { bit.set(k, value); reference[k] = value; }
            record_nodes(nodes, static_cast<U>(n), static_cast<U>(k));
            require(bit.stored_nodes() == nodes.size() && bit.get(k) == reference[k]);
            int l = static_cast<int>(rng() % (n + 1)), r = static_cast<int>(rng() % (n + 1)); if (l > r) std::swap(l, r);
            T sum{}, prefix{}, total{};
            for (auto [i, v] : reference) {
                total = static_cast<T>(total + v);
                if (i < r) prefix = static_cast<T>(prefix + v);
                if (l <= i && i < r) sum = static_cast<T>(sum + v);
            }
            require(bit.sum(l, r) == sum && bit.sum(r) == prefix && bit.sum_all() == total);
        }
    }
}

static void invalid_inputs() {
#ifndef SKIP_RUNTIME_CHECKS
    auto throws = [](auto f) { bool thrown = false; try { f(); } catch (const std::invalid_argument&) { thrown = true; } catch (const std::out_of_range&) { thrown = true; } require(thrown); };
    throws([] { DynamicFenwickTree<int, I> bit(-1); });
    DynamicFenwickTree<int, I> bit(7);
    throws([&] { bit.add(-1, 1); }); throws([&] { bit.add(7, 1); });
    throws([&] { bit.get(-1); }); throws([&] { bit.get(7); });
    throws([&] { bit.set(-1, 1); }); throws([&] { bit.set(7, 1); });
    throws([&] { bit.sum(-1); }); throws([&] { bit.sum(8); });
    throws([&] { bit.sum(5, 4); }); throws([&] { bit.sum(-1, 4); });
    throws([&] { bit.reset(-1); }); require(bit.size() == 7 && bit.sum_all() == 0);
    DynamicFenwickTree<unsigned, I> zero;
    throws([&] { zero.add(0, 1); }); throws([&] { zero.get(0); });
#endif
}

int main() {
    small_and_dense();
    sparse_boundary<std::uint8_t>(255);
    sparse_boundary<std::int8_t>(127);
    sparse_boundary<std::uint32_t>(UINT32_MAX);
    sparse_boundary<std::int64_t>(INT64_MAX);
    sparse_boundary<std::uint64_t>(UINT64_MAX);
    sparse_boundary<int>(1000000);
    sparse_boundary<int>(1000001);
    generic_values(); ownership();
    unsigned_values<std::uint8_t>(); unsigned_values<std::uint32_t>(); unsigned_values<std::uint64_t>();
    invalid_inputs();
    std::printf("dynamic_fenwick_test: %llu checks passed\n", static_cast<unsigned long long>(checks));
}
