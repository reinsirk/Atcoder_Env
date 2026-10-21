// SPDX-License-Identifier: CC0-1.0
// Original Source: https://nyaannyaan.github.io/library/data-structure/rollback-union-find.hpp.html
#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#ifndef REINSIRK_ROLLBACK_UNION_FIND_DEFINED
#define REINSIRK_ROLLBACK_UNION_FIND_DEFINED
// RollbackUnionFind(n=0): n要素を構築。O(n)時間・領域。コピーは履歴を含め独立、移動元は空。
// find(i)/root(i)/leader(i): 代表、same(a,b)/issame(a,b): 同一成分か、size(i): 成分サイズ。O(log n)。
// unite(a,b)/merge(a,b): 新規結合ならtrue。失敗した結合も履歴1回分を消費。O(log n)。
// undo(): 結合呼出し1回を戻す。O(1)。履歴が必要。get_state(): 現在の呼出し回数、O(1)。
// snapshot(): 現在位置を保存。rollback(state=-1): 指定位置/保存位置まで戻す。戻す回数に比例。
// size()/component_count(): 全要素数/成分数、O(1)。groups(): 成分別の頂点昇順一覧、O(n log n)。
// swap(other): 履歴を含めて内容を交換。O(1)。
// reserve_history(k): k回分の履歴容量を確保。履歴・代表の直接書換えは不可。
class RollbackUnionFind {
    struct Change { int x, y, dx, dy; };
    std::vector<int> data;
    std::vector<Change> history;
    int inner_snap = 0;
    int components_ = 0;
    static std::size_t checked_size(std::size_t n) {
        if (n > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("RollbackUnionFind: too many vertices");
        return n;
    }
    void check(int i) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (static_cast<std::size_t>(i) >= data.size()) throw std::out_of_range("RollbackUnionFind: vertex");
#else
        (void)i;
#endif
    }
    int leader_unchecked(int i) const { while (data[i] >= 0) i = data[i]; return i; }
    void undo_last() noexcept {
        const auto c = history.back(); history.pop_back();
        data[c.x] = c.dx; data[c.y] = c.dy;
        components_ += (c.x != c.y);
    }
public:
    explicit RollbackUnionFind(std::size_t n = 0)
        : data(checked_size(n), -1), components_(static_cast<int>(n)) { history.reserve(std::min(n, std::size_t{64})); }
    RollbackUnionFind(const RollbackUnionFind&) = default;
    RollbackUnionFind& operator=(const RollbackUnionFind& other) {
        if (this != &other) { RollbackUnionFind copy(other); swap(copy); }
        return *this;
    }
    void swap(RollbackUnionFind& other) noexcept {
        data.swap(other.data); history.swap(other.history);
        std::swap(inner_snap, other.inner_snap); std::swap(components_, other.components_);
    }
    RollbackUnionFind(RollbackUnionFind&& other) noexcept
        : data(std::move(other.data)), history(std::move(other.history)),
          inner_snap(std::exchange(other.inner_snap, 0)), components_(std::exchange(other.components_, 0)) {
        other.data.clear(); other.history.clear();
    }
    RollbackUnionFind& operator=(RollbackUnionFind&& other) noexcept {
        if (this != &other) {
            data = std::move(other.data); history = std::move(other.history);
            inner_snap = std::exchange(other.inner_snap, 0); components_ = std::exchange(other.components_, 0);
            other.data.clear(); other.history.clear();
        }
        return *this;
    }
    int find(int i) const { check(i); return leader_unchecked(i); }
    int root(int i) const { return find(i); }
    int leader(int i) const { return find(i); }
    bool unite(int x, int y) {
        check(x); check(y); x = leader_unchecked(x); y = leader_unchecked(y);
        if (history.size() == static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("RollbackUnionFind: history overflow");
        history.push_back({x, y, data[x], data[y]});
        if (x == y) return false;
        if (data[x] > data[y]) std::swap(x, y);
        data[x] += data[y]; data[y] = x; --components_; return true;
    }
    bool merge(int a, int b) { return unite(a, b); }
    bool same(int a, int b) const { check(a); check(b); return leader_unchecked(a) == leader_unchecked(b); }
    bool issame(int a, int b) const { return same(a, b); }
    int size(int i) const { return -data[find(i)]; }
    int size() const noexcept { return static_cast<int>(data.size()); }
    int component_count() const noexcept { return components_; }
    void undo() {
#ifndef SKIP_RUNTIME_CHECKS
        if (history.empty()) throw std::out_of_range("RollbackUnionFind: empty history");
#endif
        undo_last();
        if (inner_snap > get_state()) inner_snap = get_state();
    }
    void snapshot() noexcept { inner_snap = get_state(); }
    int get_state() const noexcept { return static_cast<int>(history.size()); }
    void rollback(int state = -1) {
        if (state == -1) state = inner_snap;
#ifndef SKIP_RUNTIME_CHECKS
        if (state < 0 || state > get_state()) throw std::out_of_range("RollbackUnionFind: state");
#endif
        while (get_state() > state) undo_last();
        if (inner_snap > state) inner_snap = state;
    }
    void reserve_history(std::size_t count) {
        if (count > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("RollbackUnionFind: history overflow");
        history.reserve(count);
    }
    std::vector<std::vector<int>> groups() const {
        std::vector<int> leaders(data.size()), counts(data.size(), 0), slot(data.size());
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
