#define SKIP_RUNTIME_CHECKS
// Verify: https://judge.yosupo.jp/problem/unionfind_with_potential
// Submission: https://judge.yosupo.jp/submission/412604

// Library: DataStructure/rollback-union-findw.cpp
// SPDX-License-Identifier: CC0-1.0
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/rollback_weighted_union_find.md
//
// API一覧
// [RollbackWeightedUnionFind<T>]
// TはT(0), +, -を備えるコピー・ムーブ可能な可換加法群。consistent/unite_checkedには==も必要。
// 標準の符号付き整数Tは最終結果の範囲外でoverflow_error。結合時は保存する根間の差もT範囲内が必要。
// 符号なし整数は剰余演算。整数の容量・演算保護はSKIP_RUNTIME_CHECKSでも維持する。
// RollbackWeightedUnionFind(): 空。RollbackWeightedUnionFind(n): n>=0のn頂点を構築、O(n+1)。
// コピーはO(n+h+1)で独立（hは履歴長）。移動元は空、移動構築O(1)。
// find(v)/same(x,y)/size(v): 代表/同一成分か/成分サイズ、O(log(n+1))。get_weight(v)は根からvへの差。
// root_with_weight(v): {根, 根からvへの差}。diff(x,y): y-xの差、同一成分が前提。各O(log(n+1))。
// try_diff(x,y): 未連結ならnullopt、consistent(x,y,w): 未連結または差がwならtrue。各O(log(n+1))。
// unite(x,y,w): y-x=wを追加、新規結合ならtrue。unite_checkedはMerged/AlreadyConsistent/Contradictionを返す。
// 各追加は冗長・矛盾時も履歴1件、O(log(n+1))償却時間。経路圧縮なし。undo(): 1追加を戻す、O(1)、履歴必須。
// snapshot()/get_state(): 保存位置設定/現在の追加回数、O(1)。rollback(state=-1): 指定/保存位置へ戻す、O(1+戻す回数)。
// 保存位置より前へ戻ると保存位置も縮める。clear_history(): 連結状態と容量を保ち履歴/保存位置を消去、O(h+1)上界、noexcept。
// size()/empty()/component_count(): 全頂点数/空判定/成分数、O(1)。groups(): 成分ごとの頂点昇順一覧、O(1+n log(n+1))。
// reserve_history(k): k追加分の容量を確保、O(h+1)上界。swap(other): 履歴・保存位置ごと交換、O(1)。
// clear(): 空へ戻して容量保持、O(n+h+1)上界。reset(n)/reset(): 指定/現在の頂点数で再構築、O(旧n+旧h+n+1)上界。
// resetはコピー構築・破棄が非送出で頂点/重み容量が足りる場合に領域再利用。代入も非送出なら既存重みを0へ戻す。
// その他は一時構築後に交換。reset失敗時は論理状態を保持（予約容量は変わり得る）。
// swap(a,b): 履歴・保存位置ごと定数時間で交換、noexcept。

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
template <typename T>
struct RollbackWeightedUnionFind {
    enum class MergeResult { Merged, AlreadyConsistent, Contradiction };

private:
    static constexpr bool checked_integer =
        std::is_integral_v<T> && std::is_signed_v<T>;

    // 根までの高々O(log n)個の整数を、途中の相殺を保って加算する。
    struct SignedSum {
#if defined(__SIZEOF_INT128__) && !defined(SIRKIT_FORCE_STANDARD_ARITHMETIC)
        __extension__ using WideInteger = __int128;
        WideInteger value = 0;
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
    using ArithmeticArgument = std::conditional_t<
        std::is_trivially_copy_constructible_v<T> ||
        !requires(const T& a, const T& b) {
            static_cast<T>(a + b); static_cast<T>(a - b);
        }, T, const T&>;
    static bool try_add(ArithmeticArgument a, ArithmeticArgument b, T& out) {
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
    static bool try_subtract(ArithmeticArgument a, ArithmeticArgument b, T& out) {
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
    void undo_unchecked() noexcept {
        const auto change = history_.back();
        if (change.child >= 0) {
            const int root = parent_[change.child];
            if constexpr (std::is_integral_v<T>) weight_[change.child] = T(0);
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
    void reset(int n) {
        if constexpr (std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_destructible_v<T>) {
#ifndef SKIP_RUNTIME_CHECKS
            if (n < 0) throw std::invalid_argument("weighted union find negative size");
#endif
            const auto count = static_cast<std::size_t>(n);
            if (count > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
                count > parent_.max_size() || count > weight_.max_size())
                throw std::length_error("weighted union find vertex capacity");
            if (count > parent_.capacity() || count > weight_.capacity()) {
                RollbackWeightedUnionFind replacement(n); swap(replacement); return;
            }
            const T initial(0);
            parent_.reserve(count); weight_.reserve(count);
            history_.reserve(std::min(count, std::size_t{64}));
            history_.clear();
            parent_.resize(count, -1);
            std::fill(parent_.begin(), parent_.end(), -1);
            if constexpr (std::is_nothrow_copy_assignable_v<T>) {
                weight_.resize(count, initial);
                std::fill(weight_.begin(), weight_.end(), initial);
            } else { weight_.clear(); weight_.resize(count, initial); }
            components_ = n; snapshot_ = 0; bounded_weights_ = true;
        } else { RollbackWeightedUnionFind replacement(n); swap(replacement); }
    }
    void reset() { reset(size()); }
    friend void swap(RollbackWeightedUnionFind& a, RollbackWeightedUnionFind& b) noexcept { a.swap(b); }
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
    void clear_history() noexcept { history_.clear(); snapshot_ = 0; }
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
        std::vector<std::vector<int>> result;
        result.reserve(components_);
        if (components_ == size()) {
            for (int v = 0; v < size(); ++v) result.emplace_back(std::size_t{1}, v);
            return result;
        }
        if (components_ == 1) {
            result.emplace_back(parent_.size());
            for (int v = 0; v < size(); ++v) result[0][v] = v;
            return result;
        }
        std::vector<int> roots(parent_.size()), counts(parent_.size(), 0);
        for (int v = 0; v < size(); ++v) ++counts[roots[v] = find_unchecked(v)];
        for (int v = 0; v < size(); ++v) if (counts[v]) {
            const int count = counts[v];
            counts[v] = static_cast<int>(result.size());
            result.emplace_back(); result.back().reserve(count);
        }
        for (int v = 0; v < size(); ++v) result[counts[roots[v]]].push_back(v);
        return result;
    }

};
#endif


// SPDX-License-Identifier: CC0-1.0
// Verify I/O: Verify/current/common/fast_io.hpp
// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef SIRKIT_VERIFY_FAST_IO_DEFINED
#define SIRKIT_VERIFY_FAST_IO_DEFINED

// VerifyInput(file=stdin): 非所有のFILEから読む。別の入力方式と混用しない。
// >> 整数: 64bit以下の十進整数を読む。検査省略時は値域内の正しい入力を前提とする。
// >> char/string: 空白を飛ばし、1文字／空白までの文字列を読む。
// >> 浮動小数点: 十進文字列を変換する。通常のCロケールで使う。
// bool / !: 直前までの読み取りが成功しているか／失敗したかを返す。
class VerifyInput {
    static constexpr std::size_t buffer_size = 1 << 17;
    std::FILE* file_;
    std::array<unsigned char, buffer_size> buffer_;
    std::size_t position_ = 0, size_ = 0;
    bool good_ = true;

    int peek() {
        if (position_ == size_) {
            size_ = std::fread(buffer_.data(), 1, buffer_.size(), file_);
            position_ = 0;
            if (size_ == 0) return EOF;
        }
        return buffer_[position_];
    }

    int skip_space() {
        int c;
        while ((c = peek()) != EOF && c <= ' ') ++position_;
        if (c == EOF) good_ = false;
        return c;
    }

public:
    explicit VerifyInput(std::FILE* file = stdin) : file_(file) {}
    VerifyInput(const VerifyInput&) = delete;
    VerifyInput& operator=(const VerifyInput&) = delete;

    explicit operator bool() const noexcept { return good_; }
    bool operator!() const noexcept { return !good_; }

    template<class T>
        requires (std::is_integral_v<T> && sizeof(T) <= sizeof(std::uint64_t)
                  && !std::is_same_v<T, char>)
    VerifyInput& operator>>(T& result) {
        if (!good_) return *this;
        int c = skip_space();
        if (c == EOF) return *this;
        bool negative = false;
        if (c == '-' || c == '+') {
            negative = c == '-';
            ++position_;
            c = peek();
        }
        if (c == EOF) {
            good_ = false;
            return *this;
        }
#ifndef SKIP_RUNTIME_CHECKS
        if (c < '0' || c > '9') {
            good_ = false;
            return *this;
        }
        // 先頭の0を除けば、20桁以下の末尾直前の値はuint64_tに収まる。
        while (c == '0') {
            ++position_;
            c = peek();
        }
        std::uint64_t previous = 0;
        unsigned last_digit = 0;
        std::size_t digits = 0;
#endif
        std::uint64_t value = 0;
        while (c >= '0' && c <= '9') {
#ifndef SKIP_RUNTIME_CHECKS
            previous = value;
            last_digit = static_cast<unsigned>(c - '0');
            if (digits <= 20) ++digits;
#endif
            value = value * 10 + static_cast<unsigned>(c - '0');
            ++position_;
            c = peek();
        }
#ifndef SKIP_RUNTIME_CHECKS
        constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
        if ((c != EOF && c > ' ') || digits > 20
            || (digits == 20 && (previous > maximum / 10
                || (previous == maximum / 10 && last_digit > maximum % 10)))) {
            good_ = false;
            return *this;
        }
        std::uint64_t limit = static_cast<std::uint64_t>(std::numeric_limits<T>::max());
        if constexpr (std::is_signed_v<T>) limit += negative;
        else if (negative && value != 0) good_ = false;
        if (value > limit) good_ = false;
        if (!good_) return *this;
#endif
        if constexpr (std::is_signed_v<T>) {
            constexpr std::uint64_t minimum_magnitude =
                static_cast<std::uint64_t>(std::numeric_limits<T>::max()) + 1;
            // -(最小値)は評価せず、絶対値が最大値+1のときだけ最小値を使う。
            result = negative
                ? (value == minimum_magnitude ? std::numeric_limits<T>::min()
                                  : static_cast<T>(-static_cast<T>(value)))
                : static_cast<T>(value);
        } else result = static_cast<T>(value);
        return *this;
    }

    VerifyInput& operator>>(char& result) {
        if (!good_) return *this;
        const int c = skip_space();
        if (c != EOF) {
            result = static_cast<char>(c);
            ++position_;
        }
        return *this;
    }

    VerifyInput& operator>>(std::string& result) {
        if (!good_) return *this;
        int c = skip_space();
        if (c == EOF) return *this;
        result.clear();
        do {
            result.push_back(static_cast<char>(c));
            ++position_;
            c = peek();
        } while (c != EOF && c > ' ');
        return *this;
    }

    template<class T> requires std::is_floating_point_v<T>
    VerifyInput& operator>>(T& result) {
        std::string token;
        *this >> token;
        if (!good_) return *this;
#ifndef SKIP_RUNTIME_CHECKS
        char* end = nullptr;
        const long double value = std::strtold(token.c_str(), &end);
        if (end != token.c_str() + token.size()) good_ = false;
        else result = static_cast<T>(value);
#else
        result = static_cast<T>(std::strtold(token.c_str(), nullptr));
#endif
        return *this;
    }
};

// VerifyOutput(file=stdout): 非所有のFILEへ書く。破棄時にflushし、FILEは閉じない。
// << 整数/char/文字列/浮動小数点: 値を出力する。整数は64bit以下に対応する。
// fixed(digits): 浮動小数点を小数点以下digits桁で出力する（digits>=0）。
// flush(): バッファの内容をFILEへ渡す。bool / !で書き込み失敗を確認できる。
class VerifyOutput {
    static constexpr std::size_t buffer_size = 1 << 17;
    std::FILE* file_;
    std::array<char, buffer_size> buffer_;
    std::size_t size_ = 0;
    int precision_ = 6;
    bool fixed_ = false, good_ = true;

    void write(std::string_view text) {
        while (!text.empty() && good_) {
            if (size_ == buffer_.size()) flush();
            const auto count = std::min(text.size(), buffer_.size() - size_);
            std::memcpy(buffer_.data() + size_, text.data(), count);
            size_ += count;
            text.remove_prefix(count);
        }
    }

public:
    explicit VerifyOutput(std::FILE* file = stdout) : file_(file) {}
    VerifyOutput(const VerifyOutput&) = delete;
    VerifyOutput& operator=(const VerifyOutput&) = delete;
    ~VerifyOutput() { flush(); }

    explicit operator bool() const noexcept { return good_; }
    bool operator!() const noexcept { return !good_; }

    VerifyOutput& fixed(int digits) {
        if (digits < 0) throw std::invalid_argument("negative output precision");
        fixed_ = true;
        precision_ = digits;
        return *this;
    }

    void flush() noexcept {
        std::size_t written = 0;
        while (written < size_ && good_) {
            const auto count = std::fwrite(buffer_.data() + written, 1, size_ - written, file_);
            if (count == 0) good_ = false;
            written += count;
        }
        size_ = 0;
    }

    template<class T>
        requires (std::is_integral_v<T> && sizeof(T) <= sizeof(std::uint64_t)
                  && !std::is_same_v<T, char>)
    VerifyOutput& operator<<(T value) {
        if constexpr (std::is_same_v<T, bool>) return *this << static_cast<char>('0' + value);
        else {
            char digits[std::numeric_limits<std::uint64_t>::digits10 + 3];
            const auto end = std::to_chars(digits, digits + sizeof(digits), value).ptr;
            write(std::string_view(digits, static_cast<std::size_t>(end - digits)));
            return *this;
        }
    }

    VerifyOutput& operator<<(char value) {
        if (size_ == buffer_.size()) flush();
        if (good_) buffer_[size_++] = value;
        return *this;
    }

    VerifyOutput& operator<<(std::string_view value) { write(value); return *this; }
    VerifyOutput& operator<<(const std::string& value) { write(value); return *this; }
    VerifyOutput& operator<<(const char* value) { write(value); return *this; }

    template<class T> requires std::is_floating_point_v<T>
    VerifyOutput& operator<<(T value) {
        char local[128];
        const char* format = fixed_ ? "%.*Lf" : "%.*Lg";
        const int count = std::snprintf(local, sizeof(local), format, precision_,
                                        static_cast<long double>(value));
        if (count < 0) good_ = false;
        else if (static_cast<std::size_t>(count) < sizeof(local))
            write(std::string_view(local, static_cast<std::size_t>(count)));
        else {
            std::vector<char> text(static_cast<std::size_t>(count) + 1);
            std::snprintf(text.data(), text.size(), format, precision_, static_cast<long double>(value));
            write(std::string_view(text.data(), static_cast<std::size_t>(count)));
        }
        return *this;
    }
};

inline VerifyInput verify_in;
inline VerifyOutput verify_out;
#endif

// ACL: atcoder/modint
#ifndef ATCODER_MODINT_HPP
#define ATCODER_MODINT_HPP 1

#include <cassert>
#include <numeric>
#include <type_traits>

#ifdef _MSC_VER
#include <intrin.h>
#endif

// ACL: atcoder/internal_math
#ifndef ATCODER_INTERNAL_MATH_HPP
#define ATCODER_INTERNAL_MATH_HPP 1

#include <utility>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace atcoder {

namespace internal {

// @param m `1 <= m`
// @return x mod m
constexpr long long safe_mod(long long x, long long m) {
    x %= m;
    if (x < 0) x += m;
    return x;
}

// Fast modular multiplication by barrett reduction
// Reference: https://en.wikipedia.org/wiki/Barrett_reduction
// NOTE: reconsider after Ice Lake
struct barrett {
    unsigned int _m;
    unsigned long long im;

    // @param m `1 <= m`
    explicit barrett(unsigned int m) : _m(m), im((unsigned long long)(-1) / m + 1) {}

    // @return m
    unsigned int umod() const { return _m; }

    // @param a `0 <= a < m`
    // @param b `0 <= b < m`
    // @return `a * b % m`
    unsigned int mul(unsigned int a, unsigned int b) const {
        // [1] m = 1
        // a = b = im = 0, so okay

        // [2] m >= 2
        // im = ceil(2^64 / m)
        // -> im * m = 2^64 + r (0 <= r < m)
        // let z = a*b = c*m + d (0 <= c, d < m)
        // a*b * im = (c*m + d) * im = c*(im*m) + d*im = c*2^64 + c*r + d*im
        // c*r + d*im < m * m + m * im < m * m + 2^64 + m <= 2^64 + m * (m + 1) < 2^64 * 2
        // ((ab * im) >> 64) == c or c + 1
        unsigned long long z = a;
        z *= b;
#ifdef _MSC_VER
        unsigned long long x;
        _umul128(z, im, &x);
#else
        unsigned long long x =
            (unsigned long long)(((unsigned __int128)(z)*im) >> 64);
#endif
        unsigned long long y = x * _m;
        return (unsigned int)(z - y + (z < y ? _m : 0));
    }
};

// @param n `0 <= n`
// @param m `1 <= m`
// @return `(x ** n) % m`
constexpr long long pow_mod_constexpr(long long x, long long n, int m) {
    if (m == 1) return 0;
    unsigned int _m = (unsigned int)(m);
    unsigned long long r = 1;
    unsigned long long y = safe_mod(x, m);
    while (n) {
        if (n & 1) r = (r * y) % _m;
        y = (y * y) % _m;
        n >>= 1;
    }
    return r;
}

// Reference:
// M. Forisek and J. Jancina,
// Fast Primality Testing for Integers That Fit into a Machine Word
// @param n `0 <= n`
constexpr bool is_prime_constexpr(int n) {
    if (n <= 1) return false;
    if (n == 2 || n == 7 || n == 61) return true;
    if (n % 2 == 0) return false;
    long long d = n - 1;
    while (d % 2 == 0) d /= 2;
    constexpr long long bases[3] = {2, 7, 61};
    for (long long a : bases) {
        long long t = d;
        long long y = pow_mod_constexpr(a, t, n);
        while (t != n - 1 && y != 1 && y != n - 1) {
            y = y * y % n;
            t <<= 1;
        }
        if (y != n - 1 && t % 2 == 0) {
            return false;
        }
    }
    return true;
}
template <int n> constexpr bool is_prime = is_prime_constexpr(n);

// @param b `1 <= b`
// @return pair(g, x) s.t. g = gcd(a, b), xa = g (mod b), 0 <= x < b/g
constexpr std::pair<long long, long long> inv_gcd(long long a, long long b) {
    a = safe_mod(a, b);
    if (a == 0) return {b, 0};

    // Contracts:
    // [1] s - m0 * a = 0 (mod b)
    // [2] t - m1 * a = 0 (mod b)
    // [3] s * |m1| + t * |m0| <= b
    long long s = b, t = a;
    long long m0 = 0, m1 = 1;

    while (t) {
        long long u = s / t;
        s -= t * u;
        m0 -= m1 * u;  // |m1 * u| <= |m1| * s <= b

        // [3]:
        // (s - t * u) * |m1| + t * |m0 - m1 * u|
        // <= s * |m1| - t * u * |m1| + t * (|m0| + |m1| * u)
        // = s * |m1| + t * |m0| <= b

        auto tmp = s;
        s = t;
        t = tmp;
        tmp = m0;
        m0 = m1;
        m1 = tmp;
    }
    // by [3]: |m0| <= b/g
    // by g != b: |m0| < b/g
    if (m0 < 0) m0 += b / s;
    return {s, m0};
}

// Compile time primitive root
// @param m must be prime
// @return primitive root (and minimum in now)
constexpr int primitive_root_constexpr(int m) {
    if (m == 2) return 1;
    if (m == 167772161) return 3;
    if (m == 469762049) return 3;
    if (m == 754974721) return 11;
    if (m == 998244353) return 3;
    int divs[20] = {};
    divs[0] = 2;
    int cnt = 1;
    int x = (m - 1) / 2;
    while (x % 2 == 0) x /= 2;
    for (int i = 3; (long long)(i)*i <= x; i += 2) {
        if (x % i == 0) {
            divs[cnt++] = i;
            while (x % i == 0) {
                x /= i;
            }
        }
    }
    if (x > 1) {
        divs[cnt++] = x;
    }
    for (int g = 2;; g++) {
        bool ok = true;
        for (int i = 0; i < cnt; i++) {
            if (pow_mod_constexpr(g, (m - 1) / divs[i], m) == 1) {
                ok = false;
                break;
            }
        }
        if (ok) return g;
    }
}
template <int m> constexpr int primitive_root = primitive_root_constexpr(m);

// @param n `n < 2^32`
// @param m `1 <= m < 2^32`
// @return sum_{i=0}^{n-1} floor((ai + b) / m) (mod 2^64)
unsigned long long floor_sum_unsigned(unsigned long long n,
                                      unsigned long long m,
                                      unsigned long long a,
                                      unsigned long long b) {
    unsigned long long ans = 0;
    while (true) {
        if (a >= m) {
            ans += n * (n - 1) / 2 * (a / m);
            a %= m;
        }
        if (b >= m) {
            ans += n * (b / m);
            b %= m;
        }

        unsigned long long y_max = a * n + b;
        if (y_max < m) break;
        // y_max < m * (n + 1)
        // floor(y_max / m) <= n
        n = (unsigned long long)(y_max / m);
        b = (unsigned long long)(y_max % m);
        std::swap(m, a);
    }
    return ans;
}

}  // namespace internal

}  // namespace atcoder

#endif  // ATCODER_INTERNAL_MATH_HPP

// ACL: atcoder/internal_type_traits
#ifndef ATCODER_INTERNAL_TYPE_TRAITS_HPP
#define ATCODER_INTERNAL_TYPE_TRAITS_HPP 1

#include <cassert>
#include <numeric>
#include <type_traits>

namespace atcoder {

namespace internal {

#ifndef _MSC_VER
template <class T>
using is_signed_int128 =
    typename std::conditional<std::is_same<T, __int128_t>::value ||
                                  std::is_same<T, __int128>::value,
                              std::true_type,
                              std::false_type>::type;

template <class T>
using is_unsigned_int128 =
    typename std::conditional<std::is_same<T, __uint128_t>::value ||
                                  std::is_same<T, unsigned __int128>::value,
                              std::true_type,
                              std::false_type>::type;

template <class T>
using make_unsigned_int128 =
    typename std::conditional<std::is_same<T, __int128_t>::value,
                              __uint128_t,
                              unsigned __int128>;

template <class T>
using is_integral = typename std::conditional<std::is_integral<T>::value ||
                                                  is_signed_int128<T>::value ||
                                                  is_unsigned_int128<T>::value,
                                              std::true_type,
                                              std::false_type>::type;

template <class T>
using is_signed_int = typename std::conditional<(is_integral<T>::value &&
                                                 std::is_signed<T>::value) ||
                                                    is_signed_int128<T>::value,
                                                std::true_type,
                                                std::false_type>::type;

template <class T>
using is_unsigned_int =
    typename std::conditional<(is_integral<T>::value &&
                               std::is_unsigned<T>::value) ||
                                  is_unsigned_int128<T>::value,
                              std::true_type,
                              std::false_type>::type;

template <class T>
using to_unsigned = typename std::conditional<
    is_signed_int128<T>::value,
    make_unsigned_int128<T>,
    typename std::conditional<std::is_signed<T>::value,
                              std::make_unsigned<T>,
                              std::common_type<T>>::type>::type;

#else

template <class T> using is_integral = typename std::is_integral<T>;

template <class T>
using is_signed_int =
    typename std::conditional<is_integral<T>::value && std::is_signed<T>::value,
                              std::true_type,
                              std::false_type>::type;

template <class T>
using is_unsigned_int =
    typename std::conditional<is_integral<T>::value &&
                                  std::is_unsigned<T>::value,
                              std::true_type,
                              std::false_type>::type;

template <class T>
using to_unsigned = typename std::conditional<is_signed_int<T>::value,
                                              std::make_unsigned<T>,
                                              std::common_type<T>>::type;

#endif

template <class T>
using is_signed_int_t = std::enable_if_t<is_signed_int<T>::value>;

template <class T>
using is_unsigned_int_t = std::enable_if_t<is_unsigned_int<T>::value>;

template <class T> using to_unsigned_t = typename to_unsigned<T>::type;

}  // namespace internal

}  // namespace atcoder

#endif  // ATCODER_INTERNAL_TYPE_TRAITS_HPP

namespace atcoder {

namespace internal {

struct modint_base {};
struct static_modint_base : modint_base {};

template <class T> using is_modint = std::is_base_of<modint_base, T>;
template <class T> using is_modint_t = std::enable_if_t<is_modint<T>::value>;

}  // namespace internal

template <int m, std::enable_if_t<(1 <= m)>* = nullptr>
struct static_modint : internal::static_modint_base {
    using mint = static_modint;

  public:
    static constexpr int mod() { return m; }
    static mint raw(int v) {
        mint x;
        x._v = v;
        return x;
    }

    static_modint() : _v(0) {}
    template <class T, internal::is_signed_int_t<T>* = nullptr>
    static_modint(T v) {
        long long x = (long long)(v % (long long)(umod()));
        if (x < 0) x += umod();
        _v = (unsigned int)(x);
    }
    template <class T, internal::is_unsigned_int_t<T>* = nullptr>
    static_modint(T v) {
        _v = (unsigned int)(v % umod());
    }

    int val() const { return _v; }

    mint& operator++() {
        _v++;
        if (_v == umod()) _v = 0;
        return *this;
    }
    mint& operator--() {
        if (_v == 0) _v = umod();
        _v--;
        return *this;
    }
    mint operator++(int) {
        mint result = *this;
        ++*this;
        return result;
    }
    mint operator--(int) {
        mint result = *this;
        --*this;
        return result;
    }

    mint& operator+=(const mint& rhs) {
        _v += rhs._v;
        if (_v >= umod()) _v -= umod();
        return *this;
    }
    mint& operator-=(const mint& rhs) {
        _v -= rhs._v;
        if (_v >= umod()) _v += umod();
        return *this;
    }
    mint& operator*=(const mint& rhs) {
        unsigned long long z = _v;
        z *= rhs._v;
        _v = (unsigned int)(z % umod());
        return *this;
    }
    mint& operator/=(const mint& rhs) { return *this = *this * rhs.inv(); }

    mint operator+() const { return *this; }
    mint operator-() const { return mint() - *this; }

    mint pow(long long n) const {
        assert(0 <= n);
        mint x = *this, r = 1;
        while (n) {
            if (n & 1) r *= x;
            x *= x;
            n >>= 1;
        }
        return r;
    }
    mint inv() const {
        if (prime) {
            assert(_v);
            return pow(umod() - 2);
        } else {
            auto eg = internal::inv_gcd(_v, m);
            assert(eg.first == 1);
            return eg.second;
        }
    }

    friend mint operator+(const mint& lhs, const mint& rhs) {
        return mint(lhs) += rhs;
    }
    friend mint operator-(const mint& lhs, const mint& rhs) {
        return mint(lhs) -= rhs;
    }
    friend mint operator*(const mint& lhs, const mint& rhs) {
        return mint(lhs) *= rhs;
    }
    friend mint operator/(const mint& lhs, const mint& rhs) {
        return mint(lhs) /= rhs;
    }
    friend bool operator==(const mint& lhs, const mint& rhs) {
        return lhs._v == rhs._v;
    }
    friend bool operator!=(const mint& lhs, const mint& rhs) {
        return lhs._v != rhs._v;
    }

  private:
    unsigned int _v;
    static constexpr unsigned int umod() { return m; }
    static constexpr bool prime = internal::is_prime<m>;
};

template <int id> struct dynamic_modint : internal::modint_base {
    using mint = dynamic_modint;

  public:
    static int mod() { return (int)(bt.umod()); }
    static void set_mod(int m) {
        assert(1 <= m);
        bt = internal::barrett(m);
    }
    static mint raw(int v) {
        mint x;
        x._v = v;
        return x;
    }

    dynamic_modint() : _v(0) {}
    template <class T, internal::is_signed_int_t<T>* = nullptr>
    dynamic_modint(T v) {
        long long x = (long long)(v % (long long)(mod()));
        if (x < 0) x += mod();
        _v = (unsigned int)(x);
    }
    template <class T, internal::is_unsigned_int_t<T>* = nullptr>
    dynamic_modint(T v) {
        _v = (unsigned int)(v % mod());
    }

    int val() const { return _v; }

    mint& operator++() {
        _v++;
        if (_v == umod()) _v = 0;
        return *this;
    }
    mint& operator--() {
        if (_v == 0) _v = umod();
        _v--;
        return *this;
    }
    mint operator++(int) {
        mint result = *this;
        ++*this;
        return result;
    }
    mint operator--(int) {
        mint result = *this;
        --*this;
        return result;
    }

    mint& operator+=(const mint& rhs) {
        _v += rhs._v;
        if (_v >= umod()) _v -= umod();
        return *this;
    }
    mint& operator-=(const mint& rhs) {
        _v += mod() - rhs._v;
        if (_v >= umod()) _v -= umod();
        return *this;
    }
    mint& operator*=(const mint& rhs) {
        _v = bt.mul(_v, rhs._v);
        return *this;
    }
    mint& operator/=(const mint& rhs) { return *this = *this * rhs.inv(); }

    mint operator+() const { return *this; }
    mint operator-() const { return mint() - *this; }

    mint pow(long long n) const {
        assert(0 <= n);
        mint x = *this, r = 1;
        while (n) {
            if (n & 1) r *= x;
            x *= x;
            n >>= 1;
        }
        return r;
    }
    mint inv() const {
        auto eg = internal::inv_gcd(_v, mod());
        assert(eg.first == 1);
        return eg.second;
    }

    friend mint operator+(const mint& lhs, const mint& rhs) {
        return mint(lhs) += rhs;
    }
    friend mint operator-(const mint& lhs, const mint& rhs) {
        return mint(lhs) -= rhs;
    }
    friend mint operator*(const mint& lhs, const mint& rhs) {
        return mint(lhs) *= rhs;
    }
    friend mint operator/(const mint& lhs, const mint& rhs) {
        return mint(lhs) /= rhs;
    }
    friend bool operator==(const mint& lhs, const mint& rhs) {
        return lhs._v == rhs._v;
    }
    friend bool operator!=(const mint& lhs, const mint& rhs) {
        return lhs._v != rhs._v;
    }

  private:
    unsigned int _v;
    static internal::barrett bt;
    static unsigned int umod() { return bt.umod(); }
};
template <int id> internal::barrett dynamic_modint<id>::bt(998244353);

using modint998244353 = static_modint<998244353>;
using modint1000000007 = static_modint<1000000007>;
using modint = dynamic_modint<-1>;

namespace internal {

template <class T>
using is_static_modint = std::is_base_of<internal::static_modint_base, T>;

template <class T>
using is_static_modint_t = std::enable_if_t<is_static_modint<T>::value>;

template <class> struct is_dynamic_modint : public std::false_type {};
template <int id>
struct is_dynamic_modint<dynamic_modint<id>> : public std::true_type {};

template <class T>
using is_dynamic_modint_t = std::enable_if_t<is_dynamic_modint<T>::value>;

}  // namespace internal

}  // namespace atcoder

#endif  // ATCODER_MODINT_HPP

int main() {
    using Mint = atcoder::modint998244353;
    using UnionFind = RollbackWeightedUnionFind<Mint>;
    int n, q; verify_in >> n >> q;
    UnionFind uf(n);
    while (q--) {
        int type, u, v; verify_in >> type >> u >> v;
        if (type == 0) {
            int x; verify_in >> x;
            const auto result = uf.unite_checked(v, u, Mint(x));
            verify_out << (result != UnionFind::MergeResult::Contradiction) << '\n';
        } else {
            const auto difference = uf.try_diff(v, u);
            verify_out << (difference ? difference->val() : -1) << '\n';
        }
    }
}
