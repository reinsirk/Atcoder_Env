// Verify: ローカル独立検査

// Library: Graph/LCA.cpp
// SPDX-License-Identifier: CC0-1.0
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/lca.md
//
// API一覧
// [LCA]
// 無向森のLCA。隣接リストの対称性は前提。頂点番号は[0, size())。
// LCA(): 空を作成。LCA(g, root=0): buildと同じ。
// コピー後の再構築・破棄は互いに影響しない。不変の照会表は内部共有。
// ムーブ構築/代入の移動元は空（自己代入は内容を保持）。
// build(g, root=0): rootの成分から、残りは最小未訪問頂点から構築。失敗時は元を保持。
// B=ceil(N/32)、H=最大深さ。構築O(N+E+B log(B+1))、領域O(N+B log(B+1))。
// size()/empty()/component_count()/root(): 頂点数/空か/成分数/指定根（空なら-1）。O(1)。
// parent(v)/depth_of(v)/component_of(v)/root_of(v): 親（根なら-1）/深さ/成分番号/成分根。O(1)。
// same_component(u,v)/is_ancestor(u,v): 同成分か/uがvの祖先か（自身を含む）。O(1)。
// order_index(v)/subtree_range(v)/subtree_size(v): DFS順添字/部分木の半開区間/頂点数。O(1)。
// get(u,v)/operator()(u,v): LCA、異成分なら-1。O(1)。
// get(u,v,new_root): new_rootを根としたLCA、3頂点が異成分なら-1。O(1)。
// get_distance(u,v): 辺数の距離、異成分なら-1。O(1)。
// kth_ancestor(v,k): k個上の祖先（k=0ならv）、根を越えると-1。O(log(H+2))。
// kth_on_path(u,v,k): uからvへのパスのk番目（0始まり）、異成分/範囲外なら-1。O(log(H+2))。
// clear(): 空にして領域を解放。swap(other): 内容をO(1)で交換。
// 部分木は構築時の根に対するもの。DFSの兄弟順は規定しない。
// 空のgにはroot=0を指定。検査有効時は不正な頂点・閉路・多重辺で例外。

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#ifndef SIRKIT_LCA_LADDER_DEFINED
#define SIRKIT_LCA_LADDER_DEFINED
namespace reinsirk::detail {
class AncestorLadder {
    struct Position { std::uint32_t begin; std::uint32_t index; };
    std::vector<Position> positions_;
    std::vector<int> entries_;
public:
    // parentとDFS順、深さから構築。頂点数はintの範囲、根の親は-1。O(N)。
    template<class DepthAt>
    void build(std::size_t n, const int* parent, const int* order, DepthAt depth_at,
               int* height, int* heavy) {
        if (n > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
            n > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max() / 2) ||
            n > static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) / (2 * sizeof(int))) {
            throw std::length_error("LCA: ancestor ladders too large");
        }
        AncestorLadder replacement;
        replacement.positions_.resize(n);
        replacement.entries_.reserve(2 * n);
        std::fill_n(height, n, 1);
        std::fill_n(heavy, n, -1);
        for (std::size_t i = n; i != 0;) {
            const int v = order[--i];
            const int p = parent[v];
            if (p != -1 && height[p] < height[v] + 1) {
                height[p] = height[v] + 1;
                heavy[p] = v;
            }
        }
        for (std::size_t i = 0; i < n; ++i) {
            const int head = static_cast<int>(i);
            if (parent[head] != -1 && heavy[parent[head]] == head) continue;
            const auto begin = static_cast<std::uint32_t>(replacement.entries_.size());
            const int extension = std::min(height[head], depth_at(head));
            int ancestor = head;
            for (int j = 0; j < extension; ++j) {
                ancestor = parent[ancestor];
                replacement.entries_.push_back(ancestor);
            }
            std::reverse(replacement.entries_.begin() + begin, replacement.entries_.end());
            for (int v = head; v != -1; v = heavy[v]) {
                replacement.positions_[v] = {begin, static_cast<std::uint32_t>(replacement.entries_.size())};
                replacement.entries_.push_back(v);
            }
        }
        swap(replacement);
    }
    // kはvの深さ以下。祖先を返す。O(log(H+2))。
    int climb(int v, std::uint64_t k, const int* parent) const noexcept {
        for (;;) {
            const std::uint32_t begin = positions_[v].begin;
            const std::uint32_t position = positions_[v].index;
            const std::uint32_t available = position - begin;
            if (k <= available) return entries_[position - static_cast<std::uint32_t>(k)];
            k -= static_cast<std::uint64_t>(available) + 1;
            v = parent[entries_[begin]];
        }
    }
    void swap(AncestorLadder& other) noexcept {
        positions_.swap(other.positions_);
        entries_.swap(other.entries_);
    }
    friend void swap(AncestorLadder& a, AncestorLadder& b) noexcept { a.swap(b); }
};
}
#endif

#ifndef SIRKIT_LCA_DEFINED
#define SIRKIT_LCA_DEFINED

class LCA {
    int size_ = 0;
    int root_ = -1;
    unsigned int levels_ = 0;
    bool shallow_ = true;
#if defined(__GNUC__) && !defined(__clang__)
    int root_cut_ = 0;
    struct NestedCut { int boundary = 0; int ancestor = 0; };
    NestedCut nested_cuts_[2]{};
    bool nested_enabled_ = false;
#endif
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
    struct DeepQueryData {
        std::vector<std::uint32_t> rmq_storage;
        reinsirk::detail::AncestorLadder ladder;
    };
    std::shared_ptr<const DeepQueryData> deep_;
    std::size_t block_count_ = 0;

    void check_vertex(int v) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (static_cast<unsigned int>(v) >= static_cast<unsigned int>(size_)) {
            throw std::out_of_range("LCA: vertex out of range");
        }
#else
        static_cast<void>(v);
#endif
    }

    std::size_t ancestor_index(unsigned int bit, int v) const noexcept {
#if defined(__clang__)
        return static_cast<std::size_t>(v) * static_cast<std::size_t>(levels_) + bit;
#else
        return static_cast<std::size_t>(bit) * static_cast<std::size_t>(size_) + static_cast<std::size_t>(v);
#endif
    }

    int climb(int v, std::uint64_t k) const noexcept {
        if (k == 0) return v;
        if (block_count_ != 0) return deep_->ladder.climb(v, k, up_.get());
        while (k != 0) {
#if defined(__GNUC__) && !defined(__clang__)
            const unsigned int bit = static_cast<unsigned int>(std::bit_width(k) - 1);
            v = up_[ancestor_index(bit, v)];
            k ^= std::uint64_t{1} << bit;
#else
            const unsigned int bit = static_cast<unsigned int>(std::countr_zero(k));
            v = up_[ancestor_index(bit, v)];
            k &= k - 1;
#endif
        }
        return v;
    }

#if defined(__GNUC__) && !defined(__clang__)
    void choose_nested_cuts(const int* order) noexcept {
        for (unsigned int side = 0; side < 2; ++side) {
            const int child = order[side == 0 ? 1 : root_cut_];
            const int begin = vertices_[child].entry;
            const int end = vertices_[child].end;
            const int subtree_size = end - begin;
            if (subtree_size < 128) continue;
            const int middle = order[static_cast<std::size_t>(begin) + static_cast<unsigned int>(subtree_size) / 2];
            const int grandchild = climb(middle, static_cast<unsigned int>(vertices_[middle].depth - vertices_[child].depth - 1));
            const int candidates[2] = {vertices_[grandchild].entry, vertices_[grandchild].end};
            std::int64_t best_distance = std::numeric_limits<std::int64_t>::max();
            int best = 0;
            for (const int cut : candidates) {
                if (cut <= begin + 1 || cut >= end) continue;
                std::int64_t distance = std::int64_t{2} * cut - begin - end - 1;
                if (distance < 0) distance = -distance;
                if (distance < best_distance) { best_distance = distance; best = cut; }
            }
            if (best != 0 && std::min(best - begin - 1, end - best) >= subtree_size / 8) {
                nested_cuts_[side] = {best, child};
                nested_enabled_ = true;
            }
        }
    }

    void choose_root_cut(const int* order, std::size_t root_degree) noexcept {
        if (size_ < 128 || roots_.size() != 1 || root_degree <= 1) return;
        const int middle = order[static_cast<std::size_t>(size_) / 2];
        const int child = climb(middle, static_cast<unsigned int>(vertices_[middle].depth - 1));
        const int candidates[2] = {vertices_[child].entry, vertices_[child].end};
        std::int64_t best_distance = std::numeric_limits<std::int64_t>::max();
        int best = 0;
        for (const int cut : candidates) {
            if (cut <= 1 || cut >= size_) continue;
            std::int64_t distance = std::int64_t{2} * cut - size_ - 1;
            if (distance < 0) distance = -distance;
            if (distance < best_distance) { best_distance = distance; best = cut; }
        }
        if (best != 0 && std::min(best - 1, size_ - best) >= size_ / 8) root_cut_ = best;
        if (root_cut_ != 0 && root_degree == 2) choose_nested_cuts(order);
    }
#endif

    std::uint32_t block_minimum(std::size_t left, std::size_t right) const noexcept {
        const auto* keys = deep_->rmq_storage.data() + size_;
        const auto* masks = keys + size_;
        const std::uint32_t candidates = masks[right] & (~std::uint32_t{0} << (left & 31U));
        return keys[(left & ~std::size_t{31}) + std::countr_zero(candidates)];
    }

#if defined(__GNUC__) || defined(__clang__)
    [[gnu::noinline]]
#endif
    std::uint32_t range_minimum(std::size_t left, std::size_t right,
                                std::uint32_t root_entry) const noexcept {
        const std::size_t lb = left / 32;
        const std::size_t rb = right / 32;
        if (lb == rb) return block_minimum(left, right);
        std::uint32_t result = std::numeric_limits<std::uint32_t>::max();
        const std::size_t middle_count = rb - lb - 1;
        if (middle_count != 0) {
            const unsigned int level = static_cast<unsigned int>(std::bit_width(middle_count) - 1);
            const auto* table = deep_->rmq_storage.data() + 3 * static_cast<std::size_t>(size_);
            const std::size_t offset = static_cast<std::size_t>(level) * block_count_;
            result = std::min(table[offset + lb + 1],
                              table[offset + rb - (std::size_t{1} << level)]);
            // 同成分の親のDFS添字は成分根より小さくならない。
            if (result == root_entry) return result;
        }
        return std::min(result, std::min(block_minimum(left, (lb + 1) * 32 - 1),
                                         block_minimum(rb * 32, right)));
    }

#if defined(__GNUC__) || defined(__clang__)
    [[gnu::always_inline]]
#endif
    int lca(int u, int v) const noexcept {
        if (u == v) return u;
        if (vertices_[u].component != vertices_[v].component) return -1;
        if (vertices_[u].entry > vertices_[v].entry) std::swap(u, v);
        const int target = vertices_[v].entry;
        if (target < vertices_[u].end) return u;
#if defined(__GNUC__) && !defined(__clang__)
        // 根の子部分木を分けるDFS境界をまたぐとき、LCAは根。
        if ((vertices_[u].entry < root_cut_) & (target >= root_cut_)) return root_;
        if (nested_enabled_) {
            const auto& nested = nested_cuts_[static_cast<unsigned int>(vertices_[u].entry >= root_cut_)];
            if ((vertices_[u].entry < nested.boundary) & (target >= nested.boundary)) return nested.ancestor;
        }
#endif
        if (block_count_ == 0) {
            for (unsigned int remaining = std::bit_width(static_cast<unsigned int>(vertices_[u].depth - 1)); remaining != 0;) {
                const unsigned int bit = --remaining;
                const int ancestor = up_[ancestor_index(bit, u)];
                if (vertices_[ancestor].end <= target) u = ancestor;
            }
            return up_[ancestor_index(0, u)];
        }
        const std::uint32_t lowest = range_minimum(static_cast<std::size_t>(vertices_[u].entry) + 1,
            static_cast<std::size_t>(target),
            static_cast<std::uint32_t>(vertices_[roots_[vertices_[u].component]].entry));
        return static_cast<int>(deep_->rmq_storage[lowest]);
    }

    void build_impl(const std::vector<std::vector<int>>& graph, int root) {
        const std::size_t n = graph.size();
        if (n > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
            n > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max() / 2)) {
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
        shallow_ = maximum_depth <= 1;
        std::shared_ptr<DeepQueryData> construction;
        if (maximum_depth > 256) {
            construction = std::make_shared<DeepQueryData>();
            block_count_ = n / 32 + (n % 32 != 0);
            const unsigned int block_levels = static_cast<unsigned int>(std::bit_width(block_count_));
            constexpr std::size_t maximum_count =
                static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) / sizeof(std::uint32_t);
            if (n > maximum_count / 3 || block_count_ > (maximum_count - 3 * n) / block_levels) {
                throw std::length_error("LCA: block table too large");
            }
            construction->rmq_storage.resize(3 * n + block_count_ * block_levels);
            auto* dfs = construction->rmq_storage.data();
            auto* keys = dfs + n;
            auto* masks = keys + n;
            auto* table = masks + n;
            for (std::size_t i = 0; i < n; ++i) {
                const int v = order[i];
                dfs[i] = static_cast<std::uint32_t>(v);
                // 元の頂点番号でなく、自前のDFS順で親を比較する。
                keys[i] = static_cast<std::uint32_t>(vertices_[parent[v] == -1 ? v : parent[v]].entry);
            }
            for (std::size_t block = 0; block < block_count_; ++block) {
                const std::size_t begin = block * 32;
                const std::size_t end = std::min(n, begin + 32);
                std::uint32_t mask = 0;
                for (std::size_t i = begin; i < end; ++i) {
                    while (mask != 0) {
                        const unsigned int top = 31U - static_cast<unsigned int>(std::countl_zero(mask));
                        if (keys[begin + top] < keys[i]) break;
                        mask &= ~(std::uint32_t{1} << top);
                    }
                    mask |= std::uint32_t{1} << (i - begin);
                    masks[i] = mask;
                }
                table[block] = keys[begin + std::countr_zero(mask)];
            }
            for (unsigned int level = 1; level < block_levels; ++level) {
                const std::size_t previous = static_cast<std::size_t>(level - 1) * block_count_;
                const std::size_t current = static_cast<std::size_t>(level) * block_count_;
                const std::size_t distance = std::size_t{1} << (level - 1);
                for (std::size_t block = 0; block + (std::size_t{1} << level) <= block_count_; ++block) {
                    table[current + block] = std::min(table[previous + block],
                                                      table[previous + block + distance]);
                }
            }
        }
        if (maximum_depth > 256) {
            levels_ = 1;
            up_ = std::make_unique_for_overwrite<int[]>(n);
            std::copy_n(parent, n, up_.get());
            // 親を保存した後のDFS作業領域を高さ・長い子の計算に再利用する。
            construction->ladder.build(n, up_.get(), order,
                [this](int v) { return vertices_[v].depth; }, parent, stack);
            deep_ = std::move(construction);
#if defined(__GNUC__) && !defined(__clang__)
            choose_root_cut(order, graph[root].size());
#endif
            return;
        }
        levels_ = static_cast<unsigned int>(std::bit_width(static_cast<unsigned int>(maximum_depth)));
        if (levels_ == 0) levels_ = 1;
        constexpr std::size_t maximum_table_size =
            static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) / sizeof(int);
        if (n > maximum_table_size / static_cast<std::size_t>(levels_)) {
            throw std::length_error("LCA: ancestor table too large");
        }
        up_ = std::make_unique_for_overwrite<int[]>(n * static_cast<std::size_t>(levels_));
        if (levels_ == 1) {
            // 深さ1以下なら親配列だけで祖先表が完成する。
            std::copy_n(parent, n, up_.get());
            return;
        }
        // 照会・構築の実測に合わせ、Clangは頂点順、その他は段ごとの配置。
#if defined(__clang__)
        for (std::size_t i = 0; i < n; ++i) {
            const int v = order[i];
            const std::size_t base = ancestor_index(0, v);
            up_[base] = parent[v];
            const unsigned int useful_levels = std::max(1U, static_cast<unsigned int>(
                std::bit_width(static_cast<unsigned int>(vertices_[v].depth))));
            for (unsigned int bit = 1; bit < useful_levels; ++bit) {
                const int middle = up_[base + static_cast<std::size_t>(bit - 1)];
                up_[base + static_cast<std::size_t>(bit)] =
                    up_[ancestor_index(bit - 1, middle)];
            }
            // 根の0段目は-1のまま。深さを超える高段は成分根を指す。
            std::fill_n(up_.get() + base + static_cast<std::size_t>(useful_levels),
                        levels_ - useful_levels, roots_[vertices_[v].component]);
        }
#else
        std::copy_n(parent, n, up_.get());
        // 0段目の根は-1を保ち、1段目以降は根で止める。
        for (std::size_t v = 0; v < n; ++v) {
            const int par = parent[v];
            up_[n + v] = par == -1 ? static_cast<int>(v) : (parent[par] == -1 ? par : parent[par]);
        }
        // 前段の添字は非負なので、以後は深さ判定を省ける。
        for (unsigned int bit = 2; bit < levels_; ++bit) {
            const std::size_t previous = static_cast<std::size_t>(bit - 1) * n;
            const std::size_t destination = static_cast<std::size_t>(bit) * n;
            for (std::size_t v = 0; v < n; ++v) {
                const int middle = up_[previous + v];
                up_[destination + v] = up_[previous + static_cast<std::size_t>(middle)];
            }
        }
#endif
#if defined(__GNUC__) && !defined(__clang__)
        choose_root_cut(order, graph[root].size());
#endif
    }

public:
    LCA() noexcept = default;

    explicit LCA(const std::vector<std::vector<int>>& graph, int root = 0) {
        build_impl(graph, root);
    }

    LCA(const LCA& other)
        : size_(other.size_), root_(other.root_), levels_(other.levels_), shallow_(other.shallow_),
#if defined(__GNUC__) && !defined(__clang__)
          root_cut_(other.root_cut_), nested_cuts_{other.nested_cuts_[0], other.nested_cuts_[1]},
          nested_enabled_(other.nested_enabled_),
#endif
          vertices_(other.vertices_), roots_(other.roots_), deep_(other.deep_),
          block_count_(other.block_count_) {
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
        std::swap(shallow_, other.shallow_);
#if defined(__GNUC__) && !defined(__clang__)
        std::swap(root_cut_, other.root_cut_);
        std::swap(nested_cuts_, other.nested_cuts_);
        std::swap(nested_enabled_, other.nested_enabled_);
#endif
        vertices_.swap(other.vertices_);
        roots_.swap(other.roots_);
        up_.swap(other.up_);
        deep_.swap(other.deep_);
        std::swap(block_count_, other.block_count_);
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
        return up_[ancestor_index(0, v)];
    }

    int depth_of(int v) const { check_vertex(v); return vertices_[v].depth; }
    int component_of(int v) const { check_vertex(v); return vertices_[v].component; }
    int root_of(int v) const { check_vertex(v); return roots_[vertices_[v].component]; }

    bool same_component(int u, int v) const {
        check_vertex(u);
        check_vertex(v);
        return vertices_[u].component == vertices_[v].component;
    }

    int order_index(int v) const { check_vertex(v); return vertices_[v].entry; }

    std::pair<int, int> subtree_range(int v) const {
        check_vertex(v);
        return {vertices_[v].entry, vertices_[v].end};
    }

    int subtree_size(int v) const {
        check_vertex(v);
        return vertices_[v].end - vertices_[v].entry;
    }

    bool is_ancestor(int u, int v) const {
        check_vertex(u);
        check_vertex(v);
        return vertices_[u].entry <= vertices_[v].entry && vertices_[v].entry < vertices_[u].end;
    }

#if defined(__GNUC__) || defined(__clang__)
    [[gnu::always_inline]]
#endif
    int get(int u, int v) const {
        check_vertex(u);
        check_vertex(v);
        return lca(u, v);
    }

    int operator()(int u, int v) const { return get(u, v); }

#if defined(__GNUC__) || defined(__clang__)
    [[gnu::always_inline]]
#endif
    int get(int u, int v, int new_root) const {
        check_vertex(u);
        check_vertex(v);
        check_vertex(new_root);
        if (vertices_[u].component != vertices_[v].component ||
            vertices_[u].component != vertices_[new_root].component) return -1;
        // 深さ1以下なら、異なる3頂点の根変更LCAは元の成分根。
        if (shallow_) {
            if (u == v || u == new_root) return u;
            if (v == new_root) return v;
            return roots_[vertices_[u].component];
        }
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


// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <iostream>
#include <numeric>
#include <queue>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>
int main(){
    auto check=[](bool ok){if(!ok)throw std::runtime_error("LCA reference mismatch");};std::mt19937 rng(182719);
    for(int test=0;test<80;++test){int n=test%41,root=n?rng()%n:0;std::vector<std::vector<int>>g(n);
        for(int i=1;i<n;++i)if(rng()%4){int p=rng()%i;g[p].push_back(i);g[i].push_back(p);}
        LCA lca(g,root);check(lca.size()==n);check(lca.empty()==(n==0));if(!n){check(lca.root()==-1);continue;}
        auto parents=[&](int r){std::vector<int>p(n,-2);std::queue<int>q;q.push(r);p[r]=-1;while(!q.empty()){int u=q.front();q.pop();for(int v:g[u])if(p[v]==-2){p[v]=u;q.push(v);}}return p;};
        std::vector<int>parent(n,-2),depth(n),roots(n);int components=0;
        for(int step=-1;step<n;++step){int r=step<0?root:step;if(parent[r]!=-2)continue;++components;auto p=parents(r);
            for(int v=0;v<n;++v)if(p[v]!=-2){parent[v]=p[v];roots[v]=r;int x=v,d=0;while(p[x]>=0){x=p[x];++d;}depth[v]=d;}}
        check(lca.component_count()==static_cast<std::size_t>(components));
        for(int u=0;u<n;++u){check(lca.parent(u)==parent[u]);check(lca.depth_of(u)==depth[u]);check(lca.root_of(u)==roots[u]);
            int ancestor=u;for(int k=0;k<=n;++k){check(lca.kth_ancestor(u,k)==ancestor);if(ancestor>=0)ancestor=parent[ancestor];}
            auto from=parents(u);
            for(int v=0;v<n;++v){int expected=-1;if(roots[u]==roots[v]){int a=u,b=v;while(depth[a]>depth[b])a=parent[a];while(depth[b]>depth[a])b=parent[b];while(a!=b){a=parent[a];b=parent[b];}expected=a;}
                check(lca.get(u,v)==expected);check(lca(u,v)==expected);check(lca.same_component(u,v)==(expected!=-1));
                check(lca.is_ancestor(u,v)==(expected==u));
                std::vector<int>path;if(from[v]!=-2){for(int x=v;x!=-1;x=from[x])path.push_back(x);std::reverse(path.begin(),path.end());}
                check(lca.get_distance(u,v)==(path.empty()?-1:static_cast<int>(path.size())-1));
                for(std::size_t k=0;k<=path.size();++k)check(lca.kth_on_path(u,v,k)==(k<path.size()?path[k]:-1));
                int new_root=(u+v)%n;auto np=parents(new_root);int answer=-1;if(np[u]!=-2&&np[v]!=-2){std::vector<bool>mark(n);for(int x=u;x!=-1;x=np[x])mark[x]=true;for(int x=v;x!=-1;x=np[x])if(mark[x]){answer=x;break;}}
                check(lca.get(u,v,new_root)==answer);
            }
        }
        auto copy=lca;lca.clear();check(lca.empty());check(copy.size()==n);auto moved=std::move(copy);check(copy.empty());check(moved.size()==n);
    }
    std::cout<<"OK\n";
}
