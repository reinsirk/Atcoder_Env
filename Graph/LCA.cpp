// SPDX-License-Identifier: CC0-1.0

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#ifndef REINSIRK_LCA_DEFINED
#define REINSIRK_LCA_DEFINED

// 無向森のLCA。隣接リストの対称性は前提。頂点番号は[0, size())。
// LCA(): 空を作成。LCA(g, root=0): buildと同じ。
// コピー構築/代入は独立。ムーブ構築/代入の移動元は空（自己代入は内容を保持）。
// build(g, root=0): rootの成分から、残りは最小未訪問頂点から構築。失敗時は元を保持。
// 構築O(N log(H+2)+E)、領域O(N log(H+2))。Hは構築した森の最大深さ。
// size()/empty()/component_count()/root(): 頂点数/空か/成分数/指定根（空なら-1）。O(1)。
// parent(v)/depth_of(v)/component_of(v)/root_of(v): 親（根なら-1）/深さ/成分番号/成分根。O(1)。
// same_component(u,v)/is_ancestor(u,v): 同成分か/uがvの祖先か（自身を含む）。O(1)。
// get(u,v)/operator()(u,v): LCA、異成分なら-1。O(log(H+2))。
// get(u,v,new_root): new_rootを根としたLCA、3頂点が異成分なら-1。O(log(H+2))。
// get_distance(u,v): 辺数の距離、異成分なら-1。O(log(H+2))。
// kth_ancestor(v,k): k個上の祖先（k=0ならv）、根を越えると-1。O(log(H+2))。
// kth_on_path(u,v,k): uからvへのパスのk番目（0始まり）、異成分/範囲外なら-1。O(log(H+2))。
// clear(): 空にして領域を解放。swap(other): 内容をO(1)で交換。
// 空のgにはroot=0を指定。検査有効時は不正な頂点・閉路・多重辺で例外。
class LCA {
    int size_ = 0;
    int root_ = -1;
    unsigned int levels_ = 0;
    struct Vertex {
        int depth;
        int component;
        int entry;
        int end;
        Vertex() noexcept : depth(-1) {}
    };
    std::vector<Vertex> vertices_;
    std::vector<int> roots_;
    std::unique_ptr<int[]> up_;

    void check_vertex(int v) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (static_cast<unsigned int>(v) >= static_cast<unsigned int>(size_)) {
            throw std::out_of_range("LCA: vertex out of range");
        }
#else
        static_cast<void>(v);
#endif
    }

    std::size_t row(int v) const noexcept {
        return static_cast<std::size_t>(v) * static_cast<std::size_t>(levels_);
    }

    int climb(int v, std::uint64_t k) const noexcept {
        while (k != 0) {
            const unsigned int bit = static_cast<unsigned int>(std::countr_zero(k));
            v = up_[row(v) + static_cast<std::size_t>(bit)];
            k &= k - 1;
        }
        return v;
    }

    int lca(int u, int v) const noexcept {
        if (vertices_[u].component != vertices_[v].component) return -1;
        if (vertices_[u].depth < vertices_[v].depth) std::swap(u, v);
        u = climb(u, static_cast<unsigned int>(vertices_[u].depth - vertices_[v].depth));
        if (u == v) return u;
        for (unsigned int remaining = static_cast<unsigned int>(
                 std::bit_width(static_cast<unsigned int>(vertices_[u].depth)));
             remaining != 0;) {
            const unsigned int bit = --remaining;
            const int a = up_[row(u) + static_cast<std::size_t>(bit)];
            const int b = up_[row(v) + static_cast<std::size_t>(bit)];
            if (a != b) {
                u = a;
                v = b;
            }
        }
        return up_[row(u)];
    }

    void build_impl(const std::vector<std::vector<int>>& graph, int root) {
        const std::size_t n = graph.size();
        if (n > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw std::length_error("LCA: too many vertices");
        }
#ifndef SKIP_RUNTIME_CHECKS
        if ((n == 0 && root != 0) ||
            (n != 0 && (root < 0 || static_cast<std::size_t>(root) >= n))) {
            throw std::out_of_range("LCA: root out of range");
        }
#endif
        if (n == 0) return;
        size_ = static_cast<int>(n);
        root_ = root;
        vertices_.resize(n);
        if (n > std::numeric_limits<std::size_t>::max() / (3 * sizeof(int))) {
            throw std::length_error("LCA: temporary storage too large");
        }
        auto temporary = std::make_unique_for_overwrite<int[]>(3 * n);
        int* const parent = temporary.get();
        int* const stack = parent + n;
        int* const order = stack + n;
        std::size_t stack_size = 0;
        std::size_t order_size = 0;
        int maximum_depth = 0;
        for (int candidate = -1; candidate < size_ && order_size != n; ++candidate) {
            const int start = candidate == -1 ? root : candidate;
            if (vertices_[start].depth != -1) continue;
            const int component = static_cast<int>(roots_.size());
            roots_.push_back(start);
            parent[start] = -1;
            vertices_[start].depth = 0;
            vertices_[start].component = component;
            stack[stack_size++] = start;
            while (stack_size != 0) {
                const int v = stack[--stack_size];
                vertices_[v].entry = static_cast<int>(order_size);
                vertices_[v].end = 1;
                order[order_size++] = v;
#ifndef SKIP_RUNTIME_CHECKS
                bool parent_seen = false;
#endif
                for (const int to : graph[v]) {
#ifndef SKIP_RUNTIME_CHECKS
                    if (static_cast<unsigned int>(to) >= static_cast<unsigned int>(size_)) {
                        throw std::out_of_range("LCA: endpoint out of range");
                    }
#endif
                    if (to == parent[v]) {
#ifndef SKIP_RUNTIME_CHECKS
                        if (parent_seen) {
                            throw std::invalid_argument("LCA: parallel edges");
                        }
                        parent_seen = true;
#endif
                        continue;
                    }
                    if (vertices_[to].depth != -1) {
#ifndef SKIP_RUNTIME_CHECKS
                        throw std::invalid_argument("LCA: cycle or parallel edges");
#else
                        continue;
#endif
                    }
                    parent[to] = v;
                    vertices_[to].depth = vertices_[v].depth + 1;
                    vertices_[to].component = component;
                    if (maximum_depth < vertices_[to].depth) maximum_depth = vertices_[to].depth;
                    stack[stack_size++] = to;
                }
            }
        }
        for (std::size_t i = n; i != 0;) {
            const int v = order[--i];
            if (parent[v] != -1) vertices_[parent[v]].end += vertices_[v].end;
            vertices_[v].end += vertices_[v].entry;
        }
        levels_ = static_cast<unsigned int>(std::bit_width(static_cast<unsigned int>(maximum_depth)));
        if (levels_ == 0) levels_ = 1;
        constexpr std::size_t maximum_table_size =
            static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) / sizeof(int);
        if (n > maximum_table_size / static_cast<std::size_t>(levels_)) {
            throw std::length_error("LCA: ancestor table too large");
        }
        up_ = std::make_unique_for_overwrite<int[]>(n * static_cast<std::size_t>(levels_));
        for (std::size_t i = 0; i < n; ++i) {
            const int v = order[i];
            const std::size_t base = row(v);
            up_[base] = parent[v];
            const unsigned int useful_levels = std::max(1U, static_cast<unsigned int>(
                std::bit_width(static_cast<unsigned int>(vertices_[v].depth))));
            for (unsigned int bit = 1; bit < useful_levels; ++bit) {
                const int middle = up_[base + static_cast<std::size_t>(bit - 1)];
                up_[base + static_cast<std::size_t>(bit)] =
                    up_[row(middle) + static_cast<std::size_t>(bit - 1)];
            }
            std::fill_n(up_.get() + base + static_cast<std::size_t>(useful_levels),
                        levels_ - useful_levels, -1);
        }
    }

public:
    LCA() noexcept = default;

    explicit LCA(const std::vector<std::vector<int>>& graph, int root = 0) {
        build_impl(graph, root);
    }

    LCA(const LCA& other)
        : size_(other.size_), root_(other.root_), levels_(other.levels_),
          vertices_(other.vertices_), roots_(other.roots_) {
        const std::size_t count = static_cast<std::size_t>(size_) * static_cast<std::size_t>(levels_);
        if (count != 0) {
            up_ = std::make_unique_for_overwrite<int[]>(count);
            std::copy_n(other.up_.get(), count, up_.get());
        }
    }

    LCA(LCA&& other) noexcept {
        swap(other);
    }

    LCA& operator=(const LCA& other) {
        if (this != &other) {
            LCA copy(other);
            swap(copy);
        }
        return *this;
    }

    LCA& operator=(LCA&& other) noexcept {
        if (this != &other) {
            LCA moved(std::move(other));
            swap(moved);
        }
        return *this;
    }

    void swap(LCA& other) noexcept {
        std::swap(size_, other.size_);
        std::swap(root_, other.root_);
        std::swap(levels_, other.levels_);
        vertices_.swap(other.vertices_);
        roots_.swap(other.roots_);
        up_.swap(other.up_);
    }

    friend void swap(LCA& a, LCA& b) noexcept { a.swap(b); }

    void clear() noexcept {
        LCA empty;
        swap(empty);
    }

    void build(const std::vector<std::vector<int>>& graph, int root = 0) {
        LCA replacement(graph, root);
        swap(replacement);
    }

    std::size_t size() const noexcept { return static_cast<std::size_t>(size_); }
    bool empty() const noexcept { return size_ == 0; }
    std::size_t component_count() const noexcept { return roots_.size(); }
    int root() const noexcept { return root_; }

    int parent(int v) const {
        check_vertex(v);
        return up_[row(v)];
    }

    int depth_of(int v) const { check_vertex(v); return vertices_[v].depth; }
    int component_of(int v) const { check_vertex(v); return vertices_[v].component; }
    int root_of(int v) const { check_vertex(v); return roots_[vertices_[v].component]; }

    bool same_component(int u, int v) const {
        check_vertex(u);
        check_vertex(v);
        return vertices_[u].component == vertices_[v].component;
    }

    bool is_ancestor(int u, int v) const {
        check_vertex(u);
        check_vertex(v);
        return vertices_[u].entry <= vertices_[v].entry && vertices_[v].entry < vertices_[u].end;
    }

    int get(int u, int v) const {
        check_vertex(u);
        check_vertex(v);
        return lca(u, v);
    }

    int operator()(int u, int v) const { return get(u, v); }

    int get(int u, int v, int new_root) const {
        check_vertex(u);
        check_vertex(v);
        check_vertex(new_root);
        if (vertices_[u].component != vertices_[v].component ||
            vertices_[u].component != vertices_[new_root].component) return -1;
        const int a = lca(u, v);
        const int b = lca(u, new_root);
        const int c = lca(v, new_root);
        return a == b ? c : (a == c ? b : a);
    }

    int get_distance(int u, int v) const {
        check_vertex(u);
        check_vertex(v);
        const int ancestor = lca(u, v);
        if (ancestor == -1) return -1;
        return (vertices_[u].depth - vertices_[ancestor].depth) + (vertices_[v].depth - vertices_[ancestor].depth);
    }

    int kth_ancestor(int v, std::uint64_t k) const {
        check_vertex(v);
        if (k > static_cast<std::uint64_t>(vertices_[v].depth)) return -1;
        return climb(v, static_cast<unsigned int>(k));
    }

    int kth_on_path(int u, int v, std::uint64_t k) const {
        check_vertex(u);
        check_vertex(v);
        const int ancestor = lca(u, v);
        if (ancestor == -1) return -1;
        const unsigned int first = static_cast<unsigned int>(vertices_[u].depth - vertices_[ancestor].depth);
        const unsigned int second = static_cast<unsigned int>(vertices_[v].depth - vertices_[ancestor].depth);
        const std::uint64_t distance = static_cast<std::uint64_t>(first) + second;
        if (k > distance) return -1;
        if (k <= first) return climb(u, static_cast<unsigned int>(k));
        return climb(v, static_cast<unsigned int>(distance - k));
    }
};

#endif
