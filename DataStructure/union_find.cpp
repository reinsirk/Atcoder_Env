// SPDX-License-Identifier: CC0-1.0
// Original Source: https://zenn.dev/reputeless/books/standard-cpp-for-competitive-programming/viewer/union-find
#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#ifndef SIRKIT_UNION_FIND_DEFINED
#define SIRKIT_UNION_FIND_DEFINED
// UnionFind(n=0): n要素を独立成分として構築。O(n)時間・領域。頂点はintで表せる範囲。
// root(i)/leader(i): 代表頂点、unite(a,b): 結合、merge(a,b): 新規結合ならtrue。償却O(α(n))。
// issame(a,b)/same(a,b): 同一成分か、size(i): 成分の要素数。償却O(α(n))。
// size()/component_count(): 全要素数/成分数。O(1)。groups(): 頂点昇順の成分一覧。O(n α(n))。
class UnionFind {
    std::vector<int> param;
    int components_ = 0;
    static std::size_t checked_size(std::size_t n) {
        if (n > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("UnionFind: too many vertices");
        return n;
    }
    void check(int i) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (static_cast<std::size_t>(i) >= param.size())
            throw std::out_of_range("UnionFind: vertex");
#else
        (void)i;
#endif
    }
    int leader_unchecked(int i) {
        while (param[i] >= 0) {
            const int p = param[i];
            if (param[p] < 0) return p;
            i = param[i] = param[p];
        }
        return i;
    }
public:
    explicit UnionFind(std::size_t n = 0)
        : param(checked_size(n), -1), components_(static_cast<int>(n)) {}
    UnionFind(const UnionFind&) = default;
    UnionFind& operator=(const UnionFind&) = default;
    UnionFind(UnionFind&& other) noexcept
        : param(std::move(other.param)), components_(std::exchange(other.components_, 0)) { other.param.clear(); }
    UnionFind& operator=(UnionFind&& other) noexcept {
        if (this != &other) {
            param = std::move(other.param);
            components_ = std::exchange(other.components_, 0); other.param.clear();
        }
        return *this;
    }
    int root(int i) { check(i); return leader_unchecked(i); }
    int leader(int i) { return root(i); }
    bool merge(int a, int b) {
        check(a); check(b);
        a = leader_unchecked(a); b = leader_unchecked(b);
        if (a == b) return false;
        if (param[a] > param[b]) std::swap(a, b);
        param[a] += param[b]; param[b] = a; --components_;
        return true;
    }
    void unite(int a, int b) { (void)merge(a, b); }
    bool same(int a, int b) { check(a); check(b); return leader_unchecked(a) == leader_unchecked(b); }
    bool issame(int a, int b) { return same(a, b); }
    int size(int i) { return -param[root(i)]; }
    int size() const noexcept { return static_cast<int>(param.size()); }
    int component_count() const noexcept { return components_; }
    std::vector<std::vector<int>> groups() {
        std::vector<int> leaders(param.size()), counts(param.size(), 0);
        for (int i = 0; i < size(); ++i) ++counts[leaders[i] = leader_unchecked(i)];
        std::vector<std::vector<int>> result;
        result.reserve(static_cast<std::size_t>(components_));
        std::vector<int> slot(param.size());
        for (int i = 0; i < size(); ++i) if (counts[i]) {
            slot[i] = static_cast<int>(result.size());
            result.emplace_back(); result.back().reserve(counts[i]);
        }
        for (int i = 0; i < size(); ++i) result[slot[leaders[i]]].push_back(i);
        return result;
    }
};
#endif
