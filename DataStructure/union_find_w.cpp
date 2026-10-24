// SPDX-License-Identifier: CC0-1.0
// Original Source: https://zenn.dev/reputeless/books/standard-cpp-for-competitive-programming/viewer/weighted-union-find
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/weighted_union_find.md
//
// API一覧
// [UnionFindW<T>]
// UnionFindW<T>(n=0): n要素を構築、O(n+1)時間・領域、n<=INT_MAX。T{}は零、可換加法群の+/-/+=/符号反転が必要。
// Tはコピー・ムーブ可能。圧縮・結合中のTの算術と代入は非送出、符号付き整数の全途中結果は表現範囲内が前提。
// root(i)/leader(i): 代表、issame(a,b)/same(a,b): 同一成分か、size(i): 成分サイズ。償却O(α(n+1))。
// unite(a,b,w): weight(b)-weight(a)=wで未接続なら結合、戻り値なし。既接続の矛盾は無視する。
// merge(a,b,w): 同じ制約で新規結合ならtrue、既接続なら整合性によらずfalse。償却O(α(n+1))。
// constrain(a,b,w): 未接続なら結合しtrue、既接続なら整合するときtrue。==が必要、償却O(α(n+1))。
// weight(i): 根を零とした重み、diff(a,b): b-aの差。diffは接続が前提（既定の検査で未接続なら例外）。償却O(α(n+1))。
// try_diff(a,b): 接続済みなら差、未接続ならnullopt。償却O(α(n+1))。符号なし整数は型の幅の剰余演算。
// size()/empty()/component_count(): 全要素数/空か/成分数、O(1)。groups(): 頂点昇順の成分一覧、O(1+n α(n+1))償却。
// reset(n)/reset(): n要素/現在の要素数へ初期化、O(旧n+n+1)上界。非送出コピー型は容量再利用、他は一時構築後に交換。
// reset・コピー代入の失敗時は元の連結状態を保持（予約容量は変わり得る）。clear(): 空へ戻し容量保持、O(旧n+1)上界、noexcept。
// swap(other)/swap(a,b): 内容交換、O(1)、noexcept。コピーはO(n+1)で独立、コピー代入は強い例外保証。移動元は空。

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef SIRKIT_UNION_FIND_W_DEFINED
#define SIRKIT_UNION_FIND_W_DEFINED
template<class Type>
class UnionFindW {
    std::vector<int> param;
    std::vector<Type> diff_w;
    int components_ = 0;
    static std::size_t checked_size(std::size_t n) {
        if (n > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("UnionFindW: too many vertices");
        return n;
    }
    void check(int i) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (static_cast<std::size_t>(i) >= param.size()) throw std::out_of_range("UnionFindW: vertex");
#else
        (void)i;
#endif
    }
    int leader_unchecked(int i) {
        const int p = param[i];
        if (p < 0) return i;
        if (param[p] < 0) return p;
        const int r = leader_unchecked(p);
        diff_w[i] += diff_w[p];
        return param[i] = r;
    }
    bool link(int a, int b, Type w) {
        if (param[a] > param[b]) { std::swap(a, b); w = -w; }
        diff_w[b] = w;
        param[a] += param[b]; param[b] = a; --components_;
        return true;
    }
public:
    explicit UnionFindW(std::size_t n = 0)
        : param(checked_size(n), -1), diff_w(n), components_(static_cast<int>(n)) {}
    UnionFindW(const UnionFindW&) = default;
    UnionFindW& operator=(const UnionFindW& other) {
        if (this != &other) {
            if constexpr (std::is_nothrow_copy_constructible_v<Type> && std::is_nothrow_destructible_v<Type>) {
                const auto count = other.param.size();
                param.reserve(count); diff_w.reserve(count);
                param.clear(); diff_w.clear();
                param.insert(param.end(), other.param.begin(), other.param.end());
                diff_w.insert(diff_w.end(), other.diff_w.begin(), other.diff_w.end());
                components_ = other.components_;
            } else { UnionFindW copy(other); swap(copy); }
        }
        return *this;
    }
    void swap(UnionFindW& other) noexcept {
        param.swap(other.param); diff_w.swap(other.diff_w); std::swap(components_, other.components_);
    }
    UnionFindW(UnionFindW&& other) noexcept
        : param(std::move(other.param)), diff_w(std::move(other.diff_w)), components_(std::exchange(other.components_, 0)) { other.param.clear(); other.diff_w.clear(); }
    UnionFindW& operator=(UnionFindW&& other) noexcept {
        if (this != &other) {
            param = std::move(other.param); diff_w = std::move(other.diff_w);
            components_ = std::exchange(other.components_, 0); other.param.clear(); other.diff_w.clear();
        }
        return *this;
    }
    void reset(std::size_t n) {
        if constexpr (std::is_nothrow_copy_constructible_v<Type> && std::is_nothrow_destructible_v<Type>) {
            const auto count = checked_size(n);
            if (count == 0) { clear(); return; }
            const Type zero{};
            param.reserve(count); diff_w.reserve(count);
            param.clear(); diff_w.clear();
            param.resize(count, -1); diff_w.resize(count, zero);
            components_ = static_cast<int>(count);
        } else { UnionFindW replacement(n); swap(replacement); }
    }
    void reset() { reset(param.size()); }
    void clear() noexcept { param.clear(); diff_w.clear(); components_ = 0; }
    bool empty() const noexcept { return param.empty(); }
    friend void swap(UnionFindW& left, UnionFindW& right) noexcept { left.swap(right); }
    int root(int i) { check(i); return leader_unchecked(i); }
    int leader(int i) { return root(i); }
    bool merge(int a, int b, Type w) {
        check(a); check(b);
        const int ra = leader_unchecked(a), rb = leader_unchecked(b);
        if (ra == rb) return false;
        return link(ra, rb, w + diff_w[a] - diff_w[b]);
    }
    void unite(int a, int b, Type w) { (void)merge(a, b, w); }
    bool constrain(int a, int b, Type w) {
        check(a); check(b);
        const int ra = leader_unchecked(a), rb = leader_unchecked(b);
        if (ra == rb) return static_cast<Type>(diff_w[b] - diff_w[a]) == w;
        return link(ra, rb, w + diff_w[a] - diff_w[b]);
    }
    bool same(int a, int b) { check(a); check(b); return leader_unchecked(a) == leader_unchecked(b); }
    bool issame(int a, int b) { return same(a, b); }
    int size(int i) { return -param[root(i)]; }
    int size() const noexcept { return static_cast<int>(param.size()); }
    int component_count() const noexcept { return components_; }
    Type weight(int i) { (void)root(i); return diff_w[i]; }
    Type diff(int a, int b) {
        check(a); check(b);
#ifndef SKIP_RUNTIME_CHECKS
        if (leader_unchecked(a) != leader_unchecked(b)) throw std::invalid_argument("UnionFindW: disconnected");
#else
        (void)leader_unchecked(a); (void)leader_unchecked(b);
#endif
        return diff_w[b] - diff_w[a];
    }
    std::optional<Type> try_diff(int a, int b) {
        check(a); check(b);
        if (leader_unchecked(a) != leader_unchecked(b)) return std::nullopt;
        return diff_w[b] - diff_w[a];
    }
    std::vector<std::vector<int>> groups() {
        std::vector<std::vector<int>> result; result.reserve(components_);
        if (components_ == size()) {
            for (int v = 0; v < size(); ++v) result.emplace_back(std::size_t{1}, v);
            return result;
        }
        if (components_ == 1) {
            result.emplace_back(param.size());
            for (int v = 0; v < size(); ++v) {
                (void)leader_unchecked(v); result[0][v] = v;
            }
            return result;
        }
        std::vector<int> leaders(param.size()), counts(param.size(), 0);
        for (int i = 0; i < size(); ++i) ++counts[leaders[i] = leader_unchecked(i)];
        for (int i = 0; i < size(); ++i) if (counts[i]) {
            const int count = counts[i]; counts[i] = static_cast<int>(result.size());
            result.emplace_back(); result.back().reserve(count);
        }
        for (int i = 0; i < size(); ++i) result[counts[leaders[i]]].push_back(i);
        return result;
    }

};
#endif
