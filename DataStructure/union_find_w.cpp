// SPDX-License-Identifier: CC0-1.0
// Original Source: https://zenn.dev/reputeless/books/standard-cpp-for-competitive-programming/viewer/weighted-union-find
#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#ifndef REINSIRK_UNION_FIND_W_DEFINED
#define REINSIRK_UNION_FIND_W_DEFINED
// UnionFindW<T>(n=0): n要素を構築。T{}は零、加減算・符号反転・等値比較が必要。O(n)時間・領域。
// root(i)/leader(i): 代表、issame(a,b)/same(a,b): 同一成分か、size(i): 成分サイズ。償却O(α(n))。
// unite(a,b,w): weight(b)-weight(a)=wとして未接続なら結合。既接続の矛盾は従来どおり無視。
// merge(a,b,w): 同じ制約で新規結合ならtrue、既接続なら整合性にかかわらずfalse。償却O(α(n))。
// constrain(a,b,w): 未接続なら結合しtrue、既接続なら制約が整合するときtrue。償却O(α(n))。
// weight(i): 代表を零とした重み、diff(a,b): 重み差。diffは既定で未接続なら例外、検査省略時は接続が前提。
// try_diff(a,b): 接続済みなら差、未接続ならnullopt。償却O(α(n))。Tの演算結果は表現範囲内が前提。
// swap(other): 内容を交換。O(1)。コピーは独立、移動元は空。
// size()/component_count(): 全要素数/成分数、O(1)。groups(): 成分ごとの頂点昇順一覧、O(n α(n))。
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
        if (this != &other) { UnionFindW copy(other); swap(copy); }
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
        if (ra == rb) return diff_w[b] - diff_w[a] == w;
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
        std::vector<int> leaders(param.size()), counts(param.size(), 0), slot(param.size());
        for (int i = 0; i < size(); ++i) ++counts[leaders[i] = leader_unchecked(i)];
        std::vector<std::vector<int>> result; result.reserve(components_);
        for (int i = 0; i < size(); ++i) if (counts[i]) {
            slot[i] = static_cast<int>(result.size()); result.emplace_back(); result.back().reserve(counts[i]);
        }
        for (int i = 0; i < size(); ++i) result[slot[leaders[i]]].push_back(i);
        return result;
    }
};
#endif
