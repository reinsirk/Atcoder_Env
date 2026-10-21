// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

// 逆順集約を確認する非可換モノイド。作用は恒等写像。
struct VerifyStringMonoid {
    using S = std::string;
    using F = int;
    static S op(S a, S b) { return a + b; }
    static S e() { return {}; }
    static S mapping(F, S x) { return x; }
    static F composition(F, F) { return 0; }
    static F id() { return 0; }
};

int main() {
    auto require = [](bool b) { if (!b) throw std::runtime_error("dynamic/hash local verification failed"); };
    std::mt19937_64 random(20261019);
    // 1次元BIT: 検索の前提を満たす非負配列と、疎な64ビット領域。
    for (int n : {0, 1, 8, 64, 65, 129}) {
        DynamicFenwickTree<int, std::int64_t> bit(n);
        std::vector<std::int64_t> a(n);
        for (int step = 0; step < 500; ++step) {
            if (n) { int p = random() % n; a[p] = random() % 8; bit.set(p, a[p]); }
            int l = random() % (n + 1), r = random() % (n + 1);
            if (l > r) std::swap(l, r);
            require(bit.sum(l, r) == std::accumulate(a.begin() + l, a.begin() + r, std::int64_t{}));
            auto total = std::accumulate(a.begin(), a.end(), std::int64_t{});
            require(bit.sum_all() == total);
            std::int64_t target = static_cast<std::int64_t>(random() % (total + 3)) - 1;
            int lower = 0, upper = 0; std::int64_t sum = 0;
            if (target > 0) { while (lower < n && sum + a[lower] < target) sum += a[lower++]; }
            sum = 0;
            if (target >= 0) { while (upper < n && sum + a[upper] <= target) sum += a[upper++]; }
            require(bit.lower_bound(target) == lower && bit.upper_bound(target) == upper);
            const auto nodes = bit.stored_nodes();
            for (int p = 0; p < n; ++p) require(bit[p] == a[p]);
            require(bit.stored_nodes() == nodes);
        }
        auto copy = bit; bit.clear(); require(bit.sum_all() == 0);
        require(copy.sum_all() == std::accumulate(a.begin(), a.end(), std::int64_t{}));
        bit.reset(3); bit.add(2, 9); require(bit.get(2) == 9 && bit.size() == 3);
    }
    {
        constexpr std::uint64_t n = std::uint64_t{1} << 40;
        DynamicFenwickTree<std::uint64_t, std::int64_t> bit(n);
        bit.add(n - 1, 7); bit.add(123, 3);
        require(bit.sum(n) == 10 && bit.lower_bound(4) == n - 1 && bit.upper_bound(10) == n);
    }
    // 2次元BIT: 小規模の矩形照合と、直接の大きな疎座標。
    {
        DynamicFenwickTree2D<std::int64_t> bit(7, 9);
        std::int64_t a[7][9]{};
        for (int step = 0; step < 500; ++step) {
            int x = random() % 7, y = random() % 9;
            std::int64_t value = static_cast<std::int64_t>(random() % 41) - 20;
            if (step & 1) { a[x][y] += value; bit.add(x, y, value); }
            else { a[x][y] = value; bit.set(x, y, value); }
            int l = random() % 8, r = random() % 8, d = random() % 10, u = random() % 10;
            if (l > r) std::swap(l, r);
            if (d > u) std::swap(d, u);
            std::int64_t expected = 0;
            for (int i = l; i < r; ++i) for (int j = d; j < u; ++j) expected += a[i][j];
            const auto nodes = bit.stored_nodes();
            require(bit.sum(l, d, r, u) == expected && bit.get(x, y) == a[x][y]);
            require(bit.stored_nodes() == nodes);
        }
        auto copy = bit; bit.clear(); require(bit.sum_all() == 0);
        require(copy.get(1, 1) == a[1][1]);
        bit.reset(1'000'000'000, 1'000'000'000);
        bit.add(999'999'999, 900'000'000, 11); bit.add(0, 0, 3);
        require(bit.sum_all() == 14 && bit.sum(1, 1, 1'000'000'000, 1'000'000'000) == 11);
    }
    // 自動登録と併合コールバック。
    {
        DynamicUnionFind uf;
        require(uf.empty() && !uf.contains(-4));
        require(uf.add(-4) && !uf.add(-4));
        int callbacks = 0;
        uf.unite(-4, std::numeric_limits<int>::max(), [&](int root, int child) {
            require(root != child); ++callbacks;
        });
        require(uf.same(-4, std::numeric_limits<int>::max()) && uf.size(-4) == 2 && callbacks == 1);
        uf.unite(-4, -4, [&](int, int) { ++callbacks; }); require(callbacks == 1);
        uf.add(std::numeric_limits<int>::min()); require(uf.component_count() == 2 && uf.node_count() == 3);
        auto groups = uf.groups(); require(groups.size() == 2);
        auto copy = uf; uf.clear(); require(uf.empty() && copy.node_count() == 3);
    }
    // 公開HashMap: 整数操作をstd::mapと照合し、複合キーも確認。
    {
        HashMap<int, std::int64_t> table; std::map<int, std::int64_t> reference;
        for (int step = 0; step < 3000; ++step) {
            int k = static_cast<int>(random() % 129) - 64;
            if (step % 3 == 0) { table.erase(table.find(k), false); reference.erase(k); }
            else table[k] = reference[k] = static_cast<std::int64_t>(random());
            require(table.size() == reference.size());
            require(table.contains(k) == reference.contains(k));
            for (const auto& [key, value] : table) require(reference.at(key) == value);
        }
        auto copy = table; table.clear(); require(table.empty() && copy.size() == reference.size());
        auto moved = std::move(copy); require(copy.empty() && moved.size() == reference.size());
        HashMap<std::tuple<int, std::string>, int> tuples; tuples[{4, "x"}] = 8; require(tuples.at({4, "x"}) == 8);
        HashMap<std::vector<int>, int> vectors; vectors[{1, 2}] = 3; require(vectors.at({1, 2}) == 3);
        GenericHash hash(123);
        require(hash(0.0) == hash(-0.0));
        std::unordered_set<int> first{1, 2, 3}, second{3, 2, 1}; require(hash(first) == hash(second));
        reinsirk::detail::InternalHashMap<std::int64_t, int> internal;
        internal.set_default(7); internal.reserve(128); require(!internal.find(-1));
        require(internal[-1] == 7); internal[-1] += 2; require(*internal.find(-1) == 9);
        require(internal.enumerate().size() == 1 && internal.count(-1) == 1);
        auto internal_copy = internal; internal.clear(); require(internal.empty() && *internal_copy.find(-1) == 9);
    }
    // Li Chao: 区間ごとの直接評価。取得操作がノードを増やさないことも確認。
    {
        constexpr auto infinity = std::numeric_limits<std::int64_t>::max();
        DynamicLiChaoTree<std::int64_t, infinity> tree(-20, 20);
        struct Segment { std::int64_t a, b; int l, r; };
        std::vector<Segment> lines;
        for (int step = 0; step < 100; ++step) {
            std::int64_t a = static_cast<std::int64_t>(random() % 21) - 10, b = static_cast<std::int64_t>(random() % 51) - 25;
            int l = static_cast<int>(random() % 41) - 20, r = static_cast<int>(random() % 41) - 20;
            if (l > r) std::swap(l, r);
            tree.add_segment(a, b, l, r); lines.push_back({a, b, l, r});
            auto nodes = tree.stored_nodes();
            for (int x = -20; x <= 20; ++x) {
                auto expected = infinity;
                for (auto line : lines) if (line.l <= x && x <= line.r) expected = std::min(expected, line.a * x + line.b);
                require(tree.query(x) == expected);
                auto line = tree.query_line(x);
                require((!line && expected == infinity) || (line && line->get(x) == expected));
            }
            require(nodes == tree.stored_nodes());
        }
        tree.clear(); require(tree.empty() && tree.query(0) == infinity);
        tree.add_line(1'000'000'000, 2'000'000'000'000'000'000);
        require(tree.query(20) == 2'000'000'020'000'000'000);
    }
    // 1次元・2次元の包絡を全直線・全点の走査と照合。
    {
        MinLineContainer<std::int64_t> low; MaxLineContainer<std::int64_t> high;
        LineContainer2D points;
        std::vector<std::pair<std::int64_t, std::int64_t>> lines;
        for (int step = 0; step < 150; ++step) {
            std::int64_t a = static_cast<std::int64_t>(random() % 101) - 50, b = static_cast<std::int64_t>(random() % 101) - 50;
            low.add(a, b); high.add(a, b); points.add(a, b); lines.emplace_back(a, b);
            for (std::int64_t x = -20; x <= 20; ++x) {
                auto mn = std::numeric_limits<std::int64_t>::max(), mx = std::numeric_limits<std::int64_t>::min();
                for (auto [slope, intercept] : lines) { mn = std::min(mn, slope * x + intercept); mx = std::max(mx, slope * x + intercept); }
                require(low.query(x) == mn && high.query(x) == mx);
                auto [slope, intercept] = low.query_line(x); require(slope * x + intercept == mn);
                for (std::int64_t y : {-3, 0, 4}) {
                    std::int64_t lo = std::numeric_limits<std::int64_t>::max(), hi = std::numeric_limits<std::int64_t>::min();
                    for (auto [px, py] : lines) { lo = std::min(lo, x * px + y * py); hi = std::max(hi, x * px + y * py); }
                    require(points.min_rounded_ll(x, y) == lo && points.max_rounded_ll(x, y) == hi);
                    auto p = points.argmax(x, y); require(p.first * x + p.second * y == hi);
                }
            }
        }
        auto copy = points; points.clear(); require(points.empty() && !points.max_optional(1, 2) && !copy.empty());
        LineContainer2D fractional; fractional.add(0.25L, -1.5L); fractional.add(-2.75L, 3.125L);
        require(std::fabs(fractional.max_ld(0.125L, -2.5L) - 3.78125L) < 1e-15L);
        require(lc_div<std::int64_t>(-7, 3) == -3 && lc_div<std::int64_t>(7, -3) == -3);
    }
    // Heapの置換・同時挿入取り出し。
    {
        Heap heap; std::multiset<int> reference;
        for (int step = 0; step < 5000; ++step) {
            int x = static_cast<int>(random() % 101) - 50;
            unsigned type = random() % 4;
            if (reference.empty() || type == 0) { heap.push(x); reference.insert(x); }
            else if (type == 1) { require(heap.pop_value() == *reference.begin()); reference.erase(reference.begin()); }
            else if (type == 2) { require(heap.replace_top(x) == *reference.begin()); reference.erase(reference.begin()); reference.insert(x); }
            else { reference.insert(x); require(heap.push_pop(x) == *reference.begin()); reference.erase(reference.begin()); }
            require(heap.size() == reference.size() && heap.empty() == reference.empty());
            require(heap.top_or(999) == (reference.empty() ? 999 : *reference.begin()));
        }
        auto copy = heap; heap.clear(); require(heap.top() == -1 && copy.size() == reference.size());
    }
    // Treap: 値列をvectorで追跡し、区間作用・回転・照会を照合。
    {
        using A = RangeAddSum;
        lazy_implicit_treap<A::S, A::op, A::e, A::F, A::mapping, A::composition, A::id> tree;
        std::vector<std::int64_t> a;
        for (int step = 0; step < 1500; ++step) {
            int n = static_cast<int>(a.size()), l = random() % (n + 1), r = random() % (n + 1);
            if (l > r) std::swap(l, r);
            unsigned type = random() % 7;
            if (n == 0 || type == 0) { std::int64_t x = random() % 30; tree.insert_at(l, {x, 1}); a.insert(a.begin() + l, x); }
            else if (type == 1) { tree.erase(l, r); a.erase(a.begin() + l, a.begin() + r); }
            else if (type == 2) { tree.reverse(l, r); std::reverse(a.begin() + l, a.begin() + r); }
            else if (type == 3) { tree.apply(l, r, 3); for (int i = l; i < r; ++i) a[i] += 3; }
            else if (type == 4) { int m = l + random() % (r - l + 1); tree.rotate(l, m, r); std::rotate(a.begin() + l, a.begin() + m, a.begin() + r); }
            else if (type == 5) { int p = random() % n; tree.set(p, {7, 1}); a[p] = 7; }
            else require(tree.prod(l, r).sum == std::accumulate(a.begin() + l, a.begin() + r, std::int64_t{}));
            require(tree.size() == static_cast<int>(a.size()));
            require(tree.all_prod().sum == std::accumulate(a.begin(), a.end(), std::int64_t{}));
            auto values = tree.to_vector(); for (std::size_t i = 0; i < a.size(); ++i) require(values[i].sum == a[i]);
        }
        tree.clear(); tree.push_back({2, 1}); tree.push_back({3, 1}); tree.push_back({4, 1});
        require(tree.max_right(0, [](A::S x) { return x.sum <= 5; }) == 2);
        require(tree.min_left(3, [](A::S x) { return x.sum <= 7; }) == 1);
        auto copy = tree; auto moved = std::move(copy); require(copy.empty() && moved.all_prod().sum == 9);
        lazy_implicit_treap<> ordered;
        ordered.insert_sorted(3); ordered.insert_sorted(1); ordered.insert_sorted(3);
        require(ordered.lower_bound(3) == 1 && ordered.upper_bound(3) == 3 && ordered.count(3) == 2);
        ordered.erase_val(3); require(ordered.count(3) == 1); ordered.erase_max(); require(ordered.to_vector() == std::vector<std::int64_t>{1});
        using M = VerifyStringMonoid;
        lazy_implicit_treap<M::S, M::op, M::e, M::F, M::mapping, M::composition, M::id> strings(std::vector<std::string>{"a", "b", "c", "d"});
        strings.reverse(0, 4); require(strings.prod(1, 4) == "cba");
        strings.rotate(0, 2, 4); require(strings.all_prod() == "badc");
    }
    std::cout << "OK\n";
}
