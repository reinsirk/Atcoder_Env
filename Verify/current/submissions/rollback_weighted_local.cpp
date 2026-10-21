// Verify: ローカル独立検査

// Library: DataStructure/rollback-union-findw.cpp
// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef SIRKIT_ROLLBACK_WEIGHTED_UNION_FIND_DEFINED
#define SIRKIT_ROLLBACK_WEIGHTED_UNION_FIND_DEFINED
// TはT(0), +, -を備える加法群。consistent/unite_checkedのみ==も必要。
// 標準の符号付き整数Tは最終結果の範囲外でoverflow_error。符号なし整数は剰余演算。
// find/same/size(v)はO(log n)、root_with_weight/get_weight/diff/try_diff/consistentもO(log n)。
// unite(x,y,w)はpotential(y)-potential(x)=wを追加し、異なる成分を結合した場合だけtrue。
// unite_checkedはMerged/AlreadyConsistent/Contradictionを返す。各追加は冗長・矛盾時も履歴1件。
// 各追加は償却O(log n)、undoはO(1)、rollbackは戻す追加数に比例。経路圧縮なし。
// snapshotは現在位置を保存、get_stateは追加回数、rollback(-1)は保存位置へ戻る。
// 保存位置より前へ戻ると保存位置も現在位置へ縮める。空でのundoは前提違反。
// diffは同一成分が前提、try_diffは別成分ならnullopt、consistentは未連結または差がwならtrue。
// root_with_weight(v)は{根, 根からvへの差}、get_weight(v)はその差。find(v)は根。
// size(v)は成分サイズ、size/empty/component_countは全頂点数/空判定/成分数をO(1)で返す。
// 既定構築は空、構築/reset(n)はn>=0。構築O(n)、reset/clear/コピーはO(n+履歴数)。
// clearは空へ戻し容量を保持。コピーは独立、ムーブ元は空。ムーブ構築はO(1)。
// groupsは成分の頂点列をO(n log n)で返す。reserve_history(k)はk件分確保しO(履歴数)。
// swapはO(1)。入力検査はSKIP_RUNTIME_CHECKSで除外。容量と整数演算の保護は常に有効。
template <typename T>
struct RollbackWeightedUnionFind {
    enum class MergeResult { Merged, AlreadyConsistent, Contradiction };

private:
    static constexpr bool checked_integer =
        std::is_integral_v<T> && std::is_signed_v<T>;

    // 根までの高々O(log n)個の整数を、途中の相殺を保って加算する。
    struct SignedSum {
#if defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
        __int128 value = 0;
        SignedSum() = default;
        explicit SignedSum(T x) : value(x) {}
        void add(T x) { value += x; }
        void add_sum(const SignedSum& x) { value += x.value; }
        void subtract(const SignedSum& x) { value -= x.value; }
        void negate() { value = -value; }
        bool equals(T x) const { return value == x; }
        T result() const {
            if (value < std::numeric_limits<T>::min() ||
                value > std::numeric_limits<T>::max()) {
                throw std::overflow_error("weighted union find potential overflow");
            }
            return static_cast<T>(value);
        }
#else
        // 標準C++の符号と2桁の大きさ。頂点数がint以内なので2桁で足りる。
        std::uintmax_t low = 0;
        std::uintmax_t high = 0;
        bool negative = false;
        SignedSum() = default;
        explicit SignedSum(T x) {
            negative = x < 0;
            const auto u = static_cast<std::uintmax_t>(x);
            low = negative ? std::uintmax_t{0} - u : u;
        }
        void add_sum(const SignedSum& x) {
            if (negative == x.negative) {
                const auto old = low;
                low += x.low;
                high += x.high + (low < old);
            } else if (high > x.high || (high == x.high && low >= x.low)) {
                const auto old = low;
                low -= x.low;
                high -= x.high + (old < x.low);
            } else {
                const auto old = low;
                low = x.low - low;
                high = x.high - high - (x.low < old);
                negative = x.negative;
            }
            if (high == 0 && low == 0) negative = false;
        }
        void add(T x) { add_sum(SignedSum(x)); }
        void negate() { if (high != 0 || low != 0) negative = !negative; }
        void subtract(SignedSum x) { x.negate(); add_sum(x); }
        bool equals(T x) const {
            const SignedSum other(x);
            return high == 0 && low == other.low && negative == other.negative;
        }
        T result() const {
            const auto positive_limit = static_cast<std::uintmax_t>(std::numeric_limits<T>::max());
            const auto negative_limit = std::uintmax_t{0} -
                static_cast<std::uintmax_t>(std::numeric_limits<T>::min());
            if (high != 0 || low > (negative ? negative_limit : positive_limit)) {
                throw std::overflow_error("weighted union find potential overflow");
            }
            if (!negative) return static_cast<T>(low);
            if (low == negative_limit) return std::numeric_limits<T>::min();
            return static_cast<T>(-static_cast<T>(low));
        }
#endif
    };
    using Accumulator = std::conditional_t<checked_integer, SignedSum, T>;
    struct RootWeight { int root; Accumulator weight; };
    struct NarrowRootWeight { int root; T weight; };
    struct Change { int child; int previous_parent_size; };
    std::vector<int> parent_;
    std::vector<T> weight_;
    std::vector<Change> history_;
    int components_ = 0;
    int snapshot_ = 0;
    // falseはundo後も保持し、全走査なしで保守的な上限を維持する。
    bool bounded_weights_ = true;

    static Accumulator zero() {
        if constexpr (checked_integer) return SignedSum();
        else return T(0);
    }
    static void add_value(Accumulator& a, const T& b) {
        if constexpr (checked_integer) a.add(b);
        else a = static_cast<T>(a + b);
    }
    static void add_sum(Accumulator& a, const Accumulator& b) {
        if constexpr (checked_integer) a.add_sum(b);
        else a = static_cast<T>(a + b);
    }
    static void subtract(Accumulator& a, const Accumulator& b) {
        if constexpr (checked_integer) a.subtract(b);
        else a = static_cast<T>(a - b);
    }
    static void negate(Accumulator& a) {
        if constexpr (checked_integer) a.negate();
        else a = static_cast<T>(T(0) - a);
    }
    static T result(const Accumulator& a) {
        if constexpr (checked_integer) return a.result();
        else return a;
    }
    static bool equals(const Accumulator& a, const T& b) {
        if constexpr (checked_integer) return a.equals(b);
        else return a == b;
    }
#ifndef SKIP_RUNTIME_CHECKS
    void check_vertex(int v) const {
        if (static_cast<unsigned int>(v) >= parent_.size()) {
            throw std::out_of_range("weighted union find vertex");
        }
    }
#endif
    int find_unchecked(int v) const {
        while (parent_[v] >= 0) v = parent_[v];
        return v;
    }
    RootWeight root_weight_unchecked(int v) const {
        auto value = zero();
        while (parent_[v] >= 0) {
            add_value(value, weight_[v]);
            v = parent_[v];
        }
        return {v, std::move(value)};
    }
    static bool try_add(T a, T b, T& out) {
        if constexpr (checked_integer) {
#if (defined(__GNUC__) || defined(__clang__)) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
            return !__builtin_add_overflow(a, b, &out);
#else
            using U = std::make_unsigned_t<T>;
            const U ua = static_cast<U>(a), ub = static_cast<U>(b);
            const U sum = static_cast<U>(ua + ub);
            constexpr U sign = U{1} << std::numeric_limits<T>::digits;
            if (((ua ^ sum) & (ub ^ sum) & sign) != 0) return false;
            out = std::bit_cast<T>(sum);
            return true;
#endif
        } else {
            out = static_cast<T>(a + b);
            return true;
        }
    }
    static bool try_subtract(T a, T b, T& out) {
        if constexpr (checked_integer) {
#if (defined(__GNUC__) || defined(__clang__)) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
            return !__builtin_sub_overflow(a, b, &out);
#else
            using U = std::make_unsigned_t<T>;
            const U ua = static_cast<U>(a), ub = static_cast<U>(b);
            const U difference = static_cast<U>(ua - ub);
            constexpr U sign = U{1} << std::numeric_limits<T>::digits;
            if (((ua ^ ub) & (ua ^ difference) & sign) != 0) return false;
            out = std::bit_cast<T>(difference);
            return true;
#endif
        } else {
            out = static_cast<T>(a - b);
            return true;
        }
    }
    // 通常はTのまま返す。途中で範囲を超えた場合だけ広い演算で再走査する。
    NarrowRootWeight narrow_root_weight(int v) const {
        if (parent_[v] < 0) return {v, T(0)};
        T value = weight_[v];
        v = parent_[v];
        if constexpr (checked_integer) {
            // サイズ併合の深さ<=intの桁数。全辺の絶対値<=Tmax/(2*桁数)なら途中和と2経路の差も安全。
            if (bounded_weights_) {
                while (parent_[v] >= 0) {
                    value = static_cast<T>(value + weight_[v]);
                    v = parent_[v];
                }
                return {v, value};
            }
        }
        while (parent_[v] >= 0) {
            if (!try_add(value, weight_[v], value)) return {-1, T(0)};
            v = parent_[v];
        }
        return {v, std::move(value)};
    }
    void check_history_capacity() const {
        if (history_.size() >= static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
            history_.size() >= history_.max_size()) {
            throw std::length_error("weighted union find history capacity");
        }
    }
    MergeResult record_merge(int parent, int child, T gap) {
        if constexpr (checked_integer) {
            constexpr T limit = std::numeric_limits<T>::max() /
                (2 * std::numeric_limits<int>::digits);
            if (gap < -limit || gap > limit) bounded_weights_ = false;
        }
        history_.push_back({child, parent_[parent]});
        try {
            weight_[child] = std::move(gap);
        } catch (...) {
            history_.pop_back();
            throw;
        }
        parent_[parent] += parent_[child];
        parent_[child] = parent;
        --components_;
        return MergeResult::Merged;
    }
    template <bool CheckConsistency>
#if defined(__GNUC__) || defined(__clang__)
    [[gnu::noinline]]
#endif
    MergeResult unite_wide(int x, int y, const T& w) {
        auto a = root_weight_unchecked(x);
        auto b = root_weight_unchecked(y);
        if (a.root == b.root) {
            MergeResult status = MergeResult::AlreadyConsistent;
            if constexpr (CheckConsistency) {
                subtract(b.weight, a.weight);
                if (!equals(b.weight, w)) status = MergeResult::Contradiction;
            }
            history_.push_back({-1, 0});
            return status;
        }
        auto gap = zero();
        add_value(gap, w);
        add_sum(gap, a.weight);
        subtract(gap, b.weight);
        if (parent_[a.root] > parent_[b.root]) {
            std::swap(a.root, b.root);
            negate(gap);
        }
        return record_merge(a.root, b.root, result(gap));
    }
    template <bool CheckConsistency>
#if defined(__GNUC__) || defined(__clang__)
    [[gnu::always_inline]]
#endif
    MergeResult unite_impl(int x, int y, const T& w) {
#ifndef SKIP_RUNTIME_CHECKS
        check_vertex(x); check_vertex(y);
#endif
        check_history_capacity();
        if (parent_[y] == -1 && x != y) {
            const auto a = narrow_root_weight(x);
            if (a.root < 0) return unite_wide<CheckConsistency>(x, y, w);
            T gap(0);
            if (!try_add(w, a.weight, gap)) {
                auto wide_gap = zero();
                add_value(wide_gap, w);
                add_value(wide_gap, a.weight);
                gap = result(wide_gap);
            }
            return record_merge(a.root, y, std::move(gap));
        }
        auto a = narrow_root_weight(x);
        auto b = narrow_root_weight(y);
        if (a.root < 0 || b.root < 0) return unite_wide<CheckConsistency>(x, y, w);
        if (a.root == b.root) {
            MergeResult status = MergeResult::AlreadyConsistent;
            if constexpr (CheckConsistency) {
                T difference(0);
                if (!try_subtract(b.weight, a.weight, difference) || !(difference == w)) {
                    status = MergeResult::Contradiction;
                }
            }
            history_.push_back({-1, 0});
            return status;
        }
        T gap(0);
        bool fits;
        const bool reverse = parent_[a.root] > parent_[b.root];
        if (reverse) {
            fits = try_subtract(b.weight, a.weight, gap) && try_subtract(gap, w, gap);
            std::swap(a.root, b.root);
        } else {
            fits = try_add(w, a.weight, gap) && try_subtract(gap, b.weight, gap);
        }
        if (!fits) {
            auto wide_gap = zero();
            add_value(wide_gap, w);
            add_value(wide_gap, a.weight);
            auto other = zero(); add_value(other, b.weight);
            subtract(wide_gap, other);
            if (reverse) negate(wide_gap);
            gap = result(wide_gap);
        }
        return record_merge(a.root, b.root, std::move(gap));
    }
    void undo_unchecked() {
        const auto change = history_.back();
        if (change.child >= 0) {
            const int root = parent_[change.child];
            weight_[change.child] = T(0);
            parent_[change.child] = parent_[root] - change.previous_parent_size;
            parent_[root] = change.previous_parent_size;
            ++components_;
        }
        history_.pop_back();
    }
#if defined(__GNUC__) || defined(__clang__)
    [[gnu::noinline]]
#endif
    std::pair<int, T> root_with_weight_slow(int v) const {
        const auto narrow = narrow_root_weight(v);
        if (narrow.root >= 0) return {narrow.root, narrow.weight};
        const auto found = root_weight_unchecked(v);
        return {found.root, result(found.weight)};
    }
#if defined(__GNUC__) || defined(__clang__)
    [[gnu::noinline]]
#endif
    T diff_slow(int x, int y) const {
        const auto narrow_a = narrow_root_weight(x);
        const auto narrow_b = narrow_root_weight(y);
        if (narrow_a.root >= 0 && narrow_b.root >= 0) {
#ifndef SKIP_RUNTIME_CHECKS
            if (narrow_a.root != narrow_b.root) {
                throw std::invalid_argument("weighted union find disconnected vertices");
            }
#endif
            T difference(0);
            if (try_subtract(narrow_b.weight, narrow_a.weight, difference)) return difference;
        }
        const auto a = root_weight_unchecked(x);
        auto b = root_weight_unchecked(y);
#ifndef SKIP_RUNTIME_CHECKS
        if (a.root != b.root) {
            throw std::invalid_argument("weighted union find disconnected vertices");
        }
#endif
        subtract(b.weight, a.weight);
        return result(b.weight);
    }

public:
    RollbackWeightedUnionFind() = default;
    explicit RollbackWeightedUnionFind(int n) {
#ifndef SKIP_RUNTIME_CHECKS
        if (n < 0) throw std::invalid_argument("weighted union find negative size");
#endif
        const auto count = static_cast<std::size_t>(n);
        if (count > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
            count > parent_.max_size() || count > weight_.max_size()) {
            throw std::length_error("weighted union find vertex capacity");
        }
        parent_.assign(count, -1);
        weight_.assign(count, T(0));
        history_.reserve(std::min(count, std::size_t{64}));
        components_ = n;
    }
    RollbackWeightedUnionFind(const RollbackWeightedUnionFind&) = default;
    RollbackWeightedUnionFind(RollbackWeightedUnionFind&& other) noexcept { swap(other); }
    RollbackWeightedUnionFind& operator=(RollbackWeightedUnionFind other) noexcept {
        swap(other);
        return *this;
    }
    void swap(RollbackWeightedUnionFind& other) noexcept {
        parent_.swap(other.parent_);
        weight_.swap(other.weight_);
        history_.swap(other.history_);
        std::swap(components_, other.components_);
        std::swap(snapshot_, other.snapshot_);
        std::swap(bounded_weights_, other.bounded_weights_);
    }
    void reset(int n) { RollbackWeightedUnionFind replacement(n); swap(replacement); }
    void clear() {
        parent_.clear(); weight_.clear(); history_.clear();
        components_ = 0; snapshot_ = 0;
        bounded_weights_ = true;
    }
    int size() const noexcept { return static_cast<int>(parent_.size()); }
    bool empty() const noexcept { return parent_.empty(); }
    int component_count() const noexcept { return components_; }
    int find(int v) const {
#ifndef SKIP_RUNTIME_CHECKS
        check_vertex(v);
#endif
        return find_unchecked(v);
    }
    int size(int v) const { return -parent_[find(v)]; }
    bool same(int x, int y) const {
#ifndef SKIP_RUNTIME_CHECKS
        check_vertex(x); check_vertex(y);
#endif
        const int px = parent_[x], py = parent_[y];
        const int a = px < 0 ? x : px, b = py < 0 ? y : py;
        return a == b || find_unchecked(a) == find_unchecked(b);
    }
    std::pair<int, T> root_with_weight(int v) const {
#ifndef SKIP_RUNTIME_CHECKS
        check_vertex(v);
#endif
        const int p = parent_[v];
        if (p < 0) return {v, T(0)};
        if (parent_[p] < 0) return {p, weight_[v]};
        return root_with_weight_slow(v);
    }
    T get_weight(int v) const {
#ifndef SKIP_RUNTIME_CHECKS
        check_vertex(v);
#endif
        const int p = parent_[v];
        if (p < 0) return T(0);
        if (parent_[p] < 0) return weight_[v];
        return root_with_weight_slow(v).second;
    }
    std::optional<T> try_diff(int x, int y) const {
#ifndef SKIP_RUNTIME_CHECKS
        check_vertex(x); check_vertex(y);
#endif
        const auto narrow_a = narrow_root_weight(x);
        const auto narrow_b = narrow_root_weight(y);
        if (narrow_a.root >= 0 && narrow_b.root >= 0) {
            if (narrow_a.root != narrow_b.root) return std::nullopt;
            T difference(0);
            if (try_subtract(narrow_b.weight, narrow_a.weight, difference)) return difference;
        }
        const auto a = root_weight_unchecked(x);
        auto b = root_weight_unchecked(y);
        if (a.root != b.root) return std::nullopt;
        subtract(b.weight, a.weight);
        return result(b.weight);
    }
    T diff(int x, int y) const {
#ifndef SKIP_RUNTIME_CHECKS
        check_vertex(x); check_vertex(y);
#endif
        const int px = parent_[x], py = parent_[y];
        const int a = px < 0 ? x : px, b = py < 0 ? y : py;
        if (a == b) {
            T difference(0);
            if constexpr (std::is_integral_v<T>) {
                if (try_subtract(weight_[y], weight_[x], difference)) return difference;
            } else {
                const T wx = px < 0 ? T(0) : weight_[x];
                const T wy = py < 0 ? T(0) : weight_[y];
                if (try_subtract(wy, wx, difference)) return difference;
            }
        }
        return diff_slow(x, y);
    }
    bool consistent(int x, int y, const T& w) const {
#ifndef SKIP_RUNTIME_CHECKS
        check_vertex(x); check_vertex(y);
#endif
        const auto narrow_a = narrow_root_weight(x);
        const auto narrow_b = narrow_root_weight(y);
        if (narrow_a.root >= 0 && narrow_b.root >= 0) {
            if (narrow_a.root != narrow_b.root) return true;
            T difference(0);
            return try_subtract(narrow_b.weight, narrow_a.weight, difference) && difference == w;
        }
        const auto a = root_weight_unchecked(x);
        auto b = root_weight_unchecked(y);
        if (a.root != b.root) return true;
        subtract(b.weight, a.weight);
        return equals(b.weight, w);
    }
    bool unite(int x, int y, T w) { return unite_impl<false>(x, y, w) == MergeResult::Merged; }
    MergeResult unite_checked(int x, int y, T w) { return unite_impl<true>(x, y, w); }
    void reserve_history(std::size_t calls) {
        if (calls > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
            calls > history_.max_size()) {
            throw std::length_error("weighted union find history capacity");
        }
        if (calls <= history_.size()) return;
        history_.reserve(calls);
    }
    int get_state() const noexcept { return static_cast<int>(history_.size()); }
    void snapshot() noexcept { snapshot_ = get_state(); }
    void undo() {
#ifndef SKIP_RUNTIME_CHECKS
        if (history_.empty()) throw std::out_of_range("weighted union find empty history");
#endif
        undo_unchecked();
        snapshot_ = std::min(snapshot_, get_state());
    }
    void rollback(int state = -1) {
        if (state == -1) state = snapshot_;
#ifndef SKIP_RUNTIME_CHECKS
        if (state < 0 || state > get_state()) {
            throw std::out_of_range("weighted union find rollback state");
        }
#endif
        while (get_state() > state) undo_unchecked();
        snapshot_ = std::min(snapshot_, get_state());
    }
    std::vector<std::vector<int>> groups() const {
        std::vector<std::vector<int>> buckets(parent_.size());
        for (int v = 0; v < size(); ++v) buckets[find_unchecked(v)].push_back(v);
        buckets.erase(std::remove_if(buckets.begin(), buckets.end(),
            [](const auto& group) { return group.empty(); }), buckets.end());
        return buckets;
    }
};
#endif


// SPDX-License-Identifier: CC0-1.0
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
int main(){
    using UF=RollbackWeightedUnionFind<std::int64_t>;using R=UF::MergeResult;
    auto check=[](bool ok){if(!ok)throw std::runtime_error("weighted rollback mismatch");};
    UF uf(6);check(uf.unite_checked(0,1,7)==R::Merged);check(uf.unite_checked(1,2,-3)==R::Merged);
    check(uf.diff(0,2)==4);uf.snapshot();int state=uf.get_state();
    check(uf.unite_checked(0,2,4)==R::AlreadyConsistent);check(uf.unite_checked(0,2,5)==R::Contradiction);
    check(uf.get_state()==state+2);uf.undo();check(uf.get_state()==state+1);uf.rollback();check(uf.get_state()==state);
    check(uf.unite(3,4,11));check(uf.unite(2,3,9));check(uf.diff(0,4)==24);check(!uf.try_diff(0,5));
    auto copy=uf;uf.rollback(state);check(!uf.same(0,4));check(copy.diff(0,4)==24);copy.rollback(0);check(copy.component_count()==6);
    uf.reset(3);const auto mx=std::numeric_limits<std::int64_t>::max();
    check(uf.unite(0,1,mx));check(uf.unite(0,2,mx-1));check(uf.diff(1,2)==-1);
    check(uf.consistent(1,2,-1));check(!uf.consistent(1,2,1));
    uf.clear();check(uf.empty());std::cout<<"OK\n";
}
