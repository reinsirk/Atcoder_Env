// SPDX-License-Identifier: CC0-1.0
// Original Source: https://nyaannyaan.github.io/library/data-structure/persistent-union-find.hpp.html
// 参考: https://nyaannyaan.github.io/library/data-structure/persistent-array.hpp.html
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

#ifndef SIRKIT_PERSISTENT_ARRAY_DEFINED
#define SIRKIT_PERSISTENT_ARRAY_DEFINED
// PersistentArray<T,shift=4>(n=0,ID=T{}): [0,n)の初期値ID、O(2^shift)。Tはコピー構築・代入可能。
// PersistentArray(vector,ID): 配列から構築。O(n log_(2^shift)(n+1))時間、O(n+2^shift)領域。
// get(k)/get(version,k): 現在/指定版の値を返す。O(log_(2^shift)(n+1))。
// update(k,val)/update(version,k,val): 現在/指定版から分岐して現在版を更新しNode*を返す。
// updateはO(2^shift log_(2^shift)(n+1))時間・追加領域。size(): 要素数、O(1)。
// restore(version): 現在版を選択。nullptrは全要素IDの版。O(1)。
// rootは現在版の非所有ハンドル。nullptrを指定した版は全要素ID。コピー同士は保存領域を共有し現在版は独立。
// ハンドルは同じ保存領域を所有するインスタンスが残る間有効。移動元は空。全版は最後の所有者と同時に反復解放。
// 別保存領域・寿命切れのハンドルは使用不可。並行更新は外部同期が必要。1<=shift<=8。
template<typename T, int shift = 4>
class PersistentArray {
    friend struct PersistentUnionFind;
    static_assert(shift >= 1 && shift <= 8);
    static constexpr std::size_t width = std::size_t{1} << shift;
    struct Storage;
public:
    struct Node {
    private:
        friend class PersistentArray;
        std::variant<std::array<Node*, width>, std::array<T, width>> content;
#ifndef SKIP_RUNTIME_CHECKS
        const Storage* owner;
#endif
        static std::array<T, width> filled(const T& value) {
            return [&value]<std::size_t... I>(std::index_sequence<I...>) {
                return std::array<T, width>{((void)I, value)...};
            }(std::make_index_sequence<width>{});
        }
    public:
        Node(bool leaf, const T& value, const Storage* storage)
            : content(leaf ? decltype(content)(std::in_place_index<1>, filled(value))
                           : decltype(content)(std::in_place_index<0>, std::array<Node*, width>{}))
#ifndef SKIP_RUNTIME_CHECKS
              , owner(storage)
#endif
        { (void)storage; }
        Node(const Node&) = default;
    };
private:
    struct Storage { T id; std::deque<Node> nodes; explicit Storage(T value) : id(std::move(value)) {} };
    std::shared_ptr<Storage> storage_;
    std::int64_t size_ = 0;
    int levels_ = 0;
    void check_index(std::int64_t k) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (k < 0 || k >= size_) throw std::out_of_range("PersistentArray: index");
#else
        (void)k;
#endif
    }
    void check_root(Node* n) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (n && n->owner != storage_.get()) throw std::invalid_argument("PersistentArray: foreign version");
#else
        (void)n;
#endif
    }
    Node* allocate(Node* original, bool leaf) {
        if (storage_->nodes.size() == storage_->nodes.max_size())
            throw std::length_error("PersistentArray: node capacity");
        if (original) storage_->nodes.emplace_back(*original);
        else storage_->nodes.emplace_back(leaf, storage_->id, storage_.get());
        return &storage_->nodes.back();
    }
public:
    Node* root = nullptr;
    explicit PersistentArray(std::int64_t n = 0, T id = T{}) {
        if (n < 0) throw std::length_error("PersistentArray: negative size");
        size_ = n;
        std::uint64_t top = n ? static_cast<std::uint64_t>(n - 1) : 0;
        while (top >= width) { ++levels_; top >>= shift; }
        storage_ = std::make_shared<Storage>(std::move(id)); root = allocate(nullptr, levels_ == 0);
    }
    explicit PersistentArray(const std::vector<T>& values, T id = T{})
        : PersistentArray(vector_size(values), std::move(id)) {
        for (std::size_t i = 0; i < values.size(); ++i) {
            Node* node = root;
            for (int d = levels_; d > 0; --d) {
                auto& next = std::get<0>(node->content)[(i >> (d * shift)) & (width - 1)];
                if (!next) next = allocate(nullptr, d == 1);
                node = next;
            }
            std::get<1>(node->content)[i & (width - 1)] = values[i];
        }
    }
    PersistentArray(const PersistentArray&) = default;
    PersistentArray& operator=(const PersistentArray&) = default;
    PersistentArray(PersistentArray&& other) noexcept
        : storage_(std::move(other.storage_)), size_(std::exchange(other.size_, 0)),
          levels_(std::exchange(other.levels_, 0)), root(std::exchange(other.root, nullptr)) {}
    PersistentArray& operator=(PersistentArray&& other) noexcept {
        if (this != &other) {
            storage_ = std::move(other.storage_); size_ = std::exchange(other.size_, 0);
            levels_ = std::exchange(other.levels_, 0); root = std::exchange(other.root, nullptr);
        }
        return *this;
    }
    void restore(Node* r) { check_root(r); root = r; }
    std::int64_t size() const noexcept { return size_; }
    T get(Node* n, std::int64_t k) const {
        check_index(k); check_root(n); return get_unchecked(n, k);
    }
    T get(std::int64_t k) const { return get(root, k); }
    Node* update(Node* base, std::int64_t k, const T& value) {
        check_index(k); check_root(base);
        std::array<Node*, 64> path;
        std::array<std::size_t, 64> slots;
        const auto index = static_cast<std::uint64_t>(k);
        Node* n = base;
        for (int d = levels_; d > 0; --d) {
            const auto slot = (index >> (d * shift)) & (width - 1);
            path[d] = n; slots[d] = slot;
            n = n ? std::get<0>(n->content)[slot] : nullptr;
        }
        Node* result = allocate(n, true);
        std::get<1>(result->content)[index & (width - 1)] = value;
        for (int d = 1; d <= levels_; ++d) {
            Node* parent = allocate(path[d], false);
            std::get<0>(parent->content)[slots[d]] = result; result = parent;
        }
        return root = result;
    }
    Node* update(std::int64_t k, const T& value) { return update(root, k, value); }
private:
    T get_unchecked(Node* n, std::int64_t k) const {
        const auto index = static_cast<std::uint64_t>(k);
        for (int d = levels_; d > 0; --d) {
            if (!n) return storage_->id;
            n = std::get<0>(n->content)[(index >> (d * shift)) & (width - 1)];
        }
        return n ? std::get<1>(n->content)[index & (width - 1)] : storage_->id;
    }
    static std::int64_t vector_size(const std::vector<T>& values) {
        if (values.size() > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
            throw std::length_error("PersistentArray: size overflow");
        return static_cast<std::int64_t>(values.size());
    }
};
#endif

#ifndef SIRKIT_PERSISTENT_UNION_FIND_DEFINED
#define SIRKIT_PERSISTENT_UNION_FIND_DEFINED
// PersistentUnionFind(n=0): n要素の完全永続UF。初期確保O(1)。頂点はintで表せる範囲。
// find(i[,version])/root_of(i[,version])/leader(i[,version]): 代表、size(i[,version]): 成分サイズ。O(log²(n+1))。
// find_with_size(i[,version]): {代表,負の成分サイズ}。same(a,b[,version]): 同一成分か。O(log²(n+1))。
// unite(a,b[,version])/merge(a,b[,version]): 指定版/現在版から結合し新規結合ならtrue。O(log²(n+1))。
// 結合不要でも指定版が現在版になる。成功1回でO(log(n+1))ノードを追加し全版を保持する。
// get_root(): 現在版の非所有Node*、restore(version): 現在版を選択、size(): 全要素数。O(1)。
// component_count([version]): 成分数、O(n log(n+1))。groups([version]): 成分別頂点昇順一覧、O(n log²(n+1))。
// コピーは保存領域を共有し現在版は独立、移動元は空。ハンドルの寿命はPersistentArrayと同じ。
// arr/rootは互換用公開フィールド。直接更新するとUFの不変条件が壊れるため、操作は上記APIを使う。
struct PersistentUnionFind {
    PersistentArray<int> arr;
    using Node = PersistentArray<int>::Node;
    Node* root = nullptr;
    explicit PersistentUnionFind(std::size_t n = 0) : arr(checked_size(n), -1), root(arr.root) {}
    PersistentUnionFind(const PersistentUnionFind&) = default;
    PersistentUnionFind& operator=(const PersistentUnionFind&) = default;
    PersistentUnionFind(PersistentUnionFind&& other) noexcept
        : arr(std::move(other.arr)), root(std::exchange(other.root, nullptr)) {}
    PersistentUnionFind& operator=(PersistentUnionFind&& other) noexcept {
        if (this != &other) { arr = std::move(other.arr); root = std::exchange(other.root, nullptr); }
        return *this;
    }
    int size() const noexcept { return static_cast<int>(arr.size()); }
    std::pair<int,int> find_with_size(int i, Node* r) const {
        check_version(r); arr.check_index(i);
        int value = arr.get_unchecked(r, i);
        while (value >= 0) { i = value; value = arr.get_unchecked(r, i); }
        return {i, value};
    }
    std::pair<int,int> find_with_size(int i) const { return find_with_size(i, root); }
    int find(int i, Node* r) const { return find_with_size(i, r).first; }
    int find(int i) const { return find(i, root); }
    int leader(int i, Node* r) const { return find(i, r); }
    int leader(int i) const { return find(i); }
    int root_of(int i, Node* r) const { return find(i, r); }
    int root_of(int i) const { return find(i); }
    int size(int i, Node* r) const { return -find_with_size(i, r).second; }
    int size(int i) const { return size(i, root); }
    bool same(int i, int j, Node* r) const { return find(i, r) == find(j, r); }
    bool same(int i, int j) const { return same(i, j, root); }
    bool unite(int i, int j, Node* r) {
        auto [a, sa] = find_with_size(i, r); auto [b, sb] = find_with_size(j, r);
        if (a == b) { root = arr.root = r; return false; }
        if (sa > sb) { std::swap(a, b); std::swap(sa, sb); }
        const auto old = arr.root;
        try { r = arr.update(r, a, sa + sb); r = arr.update(r, b, a); }
        catch (...) { arr.root = old; throw; }
        root = r; return true;
    }
    bool unite(int i, int j) { return unite(i, j, root); }
    bool merge(int i, int j, Node* r) { return unite(i, j, r); }
    bool merge(int i, int j) { return unite(i, j); }
    Node* get_root() const noexcept { return root; }
    void restore(Node* r) { check_version(r); arr.restore(r); root = r; }
    int component_count(Node* r) const {
        check_version(r);
        int count = 0; for (int i = 0; i < size(); ++i) count += arr.get_unchecked(r, i) < 0; return count;
    }
    int component_count() const { return component_count(root); }
    std::vector<std::vector<int>> groups(Node* r) const {
        check_version(r);
        std::vector<int> leaders(size()), count(size(), 0), slot(size());
        for (int i = 0; i < size(); ++i) ++count[leaders[i] = find(i, r)];
        std::vector<std::vector<int>> result;
        for (int i = 0; i < size(); ++i) if (count[i]) {
            slot[i] = static_cast<int>(result.size()); result.emplace_back(); result.back().reserve(count[i]);
        }
        for (int i = 0; i < size(); ++i) result[slot[leaders[i]]].push_back(i);
        return result;
    }
    std::vector<std::vector<int>> groups() const { return groups(root); }
private:
    void check_version(Node* r) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (!r && size() != 0) throw std::invalid_argument("PersistentUnionFind: null version");
        arr.check_root(r);
#else
        (void)r;
#endif
    }
    static std::int64_t checked_size(std::size_t n) {
        if (n > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("PersistentUnionFind: too many vertices");
        return static_cast<std::int64_t>(n);
    }
};
#endif
