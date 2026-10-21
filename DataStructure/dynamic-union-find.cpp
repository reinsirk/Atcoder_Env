// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/data-structure/dynamic-union-find.hpp
#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include <algorithm>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef REINSIRK_INTERNAL_HASH_MAP_DEFINED
#define REINSIRK_INTERNAL_HASH_MAP_DEFINED
namespace reinsirk::detail {
// dynamic系で共有する内部用の整数ハッシュ表。参照・ポインタは再確保で無効。
// Packedは占有フラグのbit化、Denominatorは空き容量の比率1/Denominatorを指定する。
template <class Key, class Val, bool Packed = false, unsigned Denominator = 5>
class InternalHashMap {
    // InternalHashMap(): 空の表を構築する。初期配列は確保しない。
    // operator[](key): 未登録なら設定済みの初期値で追加し、値の参照を返す。
    // find(key): 値へのポインタを返す。未登録はnullptrで、追加はしない。
    // find_nonempty(key): 容量確保済みの表でfindと同じ結果を返す。内部ループ用。
    // contains(key)/contain(key): キーが登録済みならtrueを返す。
    // count(key): 登録済みなら1、未登録なら0を返す。
    // for_each(f): 各登録キーと値のconst参照にfを適用する。fは表を変更しないこと。
    // size()/empty(): 登録件数・空かどうかを返す。
    // reserve(n): n件まで再確保せず追加できる容量を準備する。
    // reallocate(): 容量を2倍にする。空なら初期容量を確保する。
    // clear(): 登録を全て削除する。確保済みの領域は再利用する。
    // set_default(value): 今後追加する要素の初期値を設定する。
    // enumerate(): 登録済みの{キー, 値}を順序不定で列挙する。
    // swap(other): 表の内容・初期値を交換する。
    static_assert(std::is_integral_v<Key> &&
                  std::is_same_v<Key, std::remove_cv_t<Key>> &&
                  !std::is_same_v<Key, bool> &&
                  sizeof(Key) <= sizeof(std::uint64_t),
                  "InternalHashMap: Key must be an integer of at most 64 bits");
    static_assert(Denominator >= 2 && Denominator <= 8);
    struct Value { Val value{}; };
    std::vector<Key> keys_;
    std::vector<Value> values_;
    using Flag = std::conditional_t<Packed, bool, std::uint8_t>;
    std::vector<Flag> used_;
    std::size_t size_ = 0;
    Val default_value_{};
    std::uint64_t salt_ = 0;
    unsigned shift_ = 61;

    static std::uint64_t mix(std::uint64_t x) noexcept {
        x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
        x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
        return x ^ (x >> 31);
    }

    static std::uint64_t initial_salt() noexcept {
        static const std::uint64_t value = mix(static_cast<std::uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count())) | std::uint64_t{1};
        return value;
    }

    std::size_t hash(Key key) const noexcept {
        return static_cast<std::size_t>((static_cast<std::uint64_t>(key) * salt_) >> shift_);
    }

    std::size_t max_capacity() const noexcept {
        return std::bit_floor(std::min({keys_.max_size(), values_.max_size(), used_.max_size()}));
    }

    void rehash(std::size_t capacity) {
        if (salt_ == 0) salt_ = initial_salt();
        if (capacity > max_capacity())
            throw std::length_error("InternalHashMap: capacity overflow");
        const unsigned next_shift = 64 - std::countr_zero(capacity);
        std::vector<Key> keys(capacity);
        std::vector<Value> values(capacity);
        std::vector<Flag> used(capacity, 0);
        for (std::size_t i = 0; i < used_.size(); ++i) {
            if (!used_[i]) continue;
            std::size_t p = static_cast<std::size_t>((static_cast<std::uint64_t>(keys_[i]) * salt_) >> next_shift);
            while (used[p]) p = (p + 1) & (capacity - 1);
            keys[p] = keys_[i];
            values[p].value = values_[i].value;
            used[p] = 1;
        }
        keys_.swap(keys);
        values_.swap(values);
        used_.swap(used);
        shift_ = next_shift;
    }

public:
    // 空の状態では配列を確保しない。
    InternalHashMap() noexcept(std::is_nothrow_default_constructible_v<Val>) = default;
    InternalHashMap(const InternalHashMap&) = default;
    InternalHashMap(InternalHashMap&& other) noexcept(std::is_nothrow_default_constructible_v<Val> &&
                                    std::is_nothrow_swappable_v<Val>) : InternalHashMap() { swap(other); }
    InternalHashMap& operator=(InternalHashMap other) {
        swap(other);
        return *this;
    }

    void swap(InternalHashMap& other) noexcept(std::is_nothrow_swappable_v<Val>) {
        using std::swap;
        swap(default_value_, other.default_value_);
        keys_.swap(other.keys_);
        values_.swap(other.values_);
        used_.swap(other.used_);
        swap(size_, other.size_);
        swap(salt_, other.salt_);
        swap(shift_, other.shift_);
    }

    // n要素まで再確保せずに格納できる容量を確保する。
    void reserve(std::size_t n) {
        if (n <= keys_.size() - keys_.size() / Denominator) return;
        std::size_t capacity = std::max(std::size_t{8}, keys_.size());
        while (n > capacity - capacity / Denominator) {
            if (capacity > max_capacity() / 2)
                throw std::length_error("InternalHashMap: capacity overflow");
            capacity *= 2;
        }
        rehash(capacity);
    }

    // 容量を2倍にする。空なら初期容量を確保する。
    void reallocate() {
        if (keys_.empty()) {
            rehash(8);
        } else {
            if (keys_.size() > max_capacity() / 2)
                throw std::length_error("InternalHashMap: capacity overflow");
            rehash(keys_.size() * 2);
        }
    }

    // 未登録なら設定済みの初期値で追加する。
    Val& operator[](Key key) {
        if (keys_.empty()) reallocate();
        std::size_t p = hash(key);
        while (used_[p]) {
            if (keys_[p] == key) return values_[p].value;
            p = (p + 1) & (keys_.size() - 1);
        }
        if (size_ == keys_.size() - keys_.size() / Denominator) {
            reallocate();
            p = hash(key);
            while (used_[p]) p = (p + 1) & (keys_.size() - 1);
        }
        keys_[p] = key;
        values_[p].value = default_value_;
        used_[p] = 1;
        ++size_;
        return values_[p].value;
    }

    // 未登録ならnullptrを返す。読取では要素を追加しない。
    const Val* find(const Key& key) const noexcept {
        if (keys_.empty()) return nullptr;
        return find_nonempty(key);
    }
    const Val* find_nonempty(const Key& key) const noexcept {
        std::size_t p = hash(key);
        while (used_[p]) {
            if (keys_[p] == key) return &values_[p].value;
            p = (p + 1) & (keys_.size() - 1);
        }
        return nullptr;
    }
    Val* find(const Key& key) noexcept {
        return const_cast<Val*>(static_cast<const InternalHashMap&>(*this).find(key));
    }

    // 登録済みのキーと値を列挙する。順序は不定。
    std::vector<std::pair<Key, Val>> enumerate() const {
        std::vector<std::pair<Key, Val>> result;
        result.reserve(size_);
        for (std::size_t i = 0; i < used_.size(); ++i)
            if (used_[i]) result.emplace_back(keys_[i], values_[i].value);
        return result;
    }

    template <class F> void for_each(F&& f) const {
        for (std::size_t i = 0; i < used_.size(); ++i)
            if (used_[i]) f(keys_[i], values_[i].value);
    }

    [[nodiscard]] bool contains(Key key) const noexcept { return find(key) != nullptr; }
    [[nodiscard]] bool contain(Key key) const noexcept { return contains(key); }
    [[nodiscard]] int count(Key key) const noexcept { return contains(key) ? 1 : 0; }
    void clear() noexcept {
        std::fill(used_.begin(), used_.end(), Flag{0});
        size_ = 0;
    }
    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    // 今後追加する要素の初期値を設定する。
    void set_default(const Val& value) { default_value_ = value; }
};

}  // namespace reinsirk::detail
#endif  // REINSIRK_INTERNAL_HASH_MAP_DEFINED


#ifndef REINSIRK_DYNAMIC_UNION_FIND_DEFINED
#define REINSIRK_DYNAMIC_UNION_FIND_DEFINED
// intの全域をキーにできる。要素は問い合わせ時に自動登録する。
// ハッシュ表の検索は期待O(1)、併合・探索は償却O(α(N))、空間O(N)。
class DynamicUnionFind {
    // DynamicUnionFind(): 空で構築する。登録数はINT_MAXまで。
    // DynamicUnionFind(n): n要素分の容量を予約して空で構築する。
    // コピーは独立した複製。ムーブ元は空になり、再利用できる。
    // data(k): 自動登録し、根なら負の要素数、非根なら親のキーを返す。
    // find(k): 自動登録し、所属する連結成分の代表キーを返す。
    // unite(x, y): 自動登録してサイズで併合し、併合したときtrueを返す。
    // unite(x, y, f): 併合後にf(新しい代表キー, 吸収した代表キー)を1回呼ぶ。
    // size(k): 自動登録し、kが所属する連結成分の要素数を返す。
    // same(x, y): 自動登録し、同じ連結成分ならtrueを返す。
    // add(k): 未登録のキーを単独の成分として追加し、追加したときtrueを返す。
    // contains(k): キーが登録済みならtrueを返す。自動登録しない。
    // node_count(): 登録済みのキーの個数をO(1)で返す。
    // component_count(): 登録済みの連結成分数をO(1)で返す。
    // empty(): 登録済みのキーがなければtrueを返す。
    // reserve(n): n要素分を予約する。成分や登録状態は変えない。
    // groups(): 全成分のキー一覧をO(N α(N))で返す。成分間の順序は未規定。
    // clear(): 全登録を消去する。確保済みの容量は再利用する。
    // dataの負の返値は、負の親キーとの区別を保証しない。判定にはfindを使う。
    struct Node {
        int key;
        // 根なら負のサイズ、非根なら親の添字。キーの符号とは独立。
        int parent;
    };
    reinsirk::detail::InternalHashMap<int, int, false, 4> indices_;
    std::vector<Node> nodes_;
    std::size_t components_ = 0;

    static void check_capacity(std::size_t count) {
        if (count > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::length_error("DynamicUnionFind: too many nodes");
    }

    int index_for(int key) {
        if (const auto* index = indices_.find(key)) return *index;
        check_capacity(nodes_.size() + 1);
        const int index = static_cast<int>(nodes_.size());
        nodes_.push_back(Node{key, -1});
        try {
            indices_[key] = index;
        } catch (...) {
            nodes_.pop_back();
            throw;
        }
        ++components_;
        return index;
    }

    int root_index(int index) noexcept {
        while (nodes_[index].parent >= 0) {
            const int parent = nodes_[index].parent;
            if (nodes_[parent].parent < 0) return parent;
            nodes_[index].parent = nodes_[parent].parent;
            index = nodes_[index].parent;
        }
        return index;
    }

    std::pair<int, int> merge_roots(int x, int y) {
        int a = root_index(index_for(x));
        int b = root_index(index_for(y));
        if (a == b) return {-1, -1};
        if (nodes_[a].parent > nodes_[b].parent) std::swap(a, b);
        // 各成分のサイズの合計は、上限検査済みの登録数以下。
        nodes_[a].parent += nodes_[b].parent;
        nodes_[b].parent = a;
        --components_;
        return {a, b};
    }

public:
    DynamicUnionFind() = default;
    explicit DynamicUnionFind(std::size_t expected_nodes) { reserve(expected_nodes); }
    DynamicUnionFind(const DynamicUnionFind&) = default;
    DynamicUnionFind& operator=(const DynamicUnionFind& other) {
        if (this == &other) return *this;
        DynamicUnionFind copy(other);
        indices_.swap(copy.indices_);
        nodes_.swap(copy.nodes_);
        std::swap(components_, copy.components_);
        return *this;
    }
    DynamicUnionFind(DynamicUnionFind&& other) noexcept
        : indices_(std::move(other.indices_)), nodes_(std::move(other.nodes_)),
          components_(std::exchange(other.components_, 0)) {
        other.nodes_.clear();
    }
    DynamicUnionFind& operator=(DynamicUnionFind&& other) noexcept {
        if (this == &other) return *this;
        indices_ = std::move(other.indices_);
        nodes_ = std::move(other.nodes_);
        components_ = std::exchange(other.components_, 0);
        other.nodes_.clear();
        return *this;
    }

    int data(int key) {
        const int index = index_for(key);
        return nodes_[index].parent < 0 ? nodes_[index].parent
                                             : nodes_[nodes_[index].parent].key;
    }

    int find(int key) { return nodes_[root_index(index_for(key))].key; }

    int unite(int x, int y) { return merge_roots(x, y).first != -1; }

    template <class F>
    int unite(int x, int y, const F& callback) {
        const auto [root, child] = merge_roots(x, y);
        if (root == -1) return false;
        callback(nodes_[root].key, nodes_[child].key);
        return true;
    }

    int size(int key) { return -nodes_[root_index(index_for(key))].parent; }
    int same(int x, int y) { return find(x) == find(y); }

    bool add(int key) {
        const auto before = nodes_.size();
        index_for(key);
        return before != nodes_.size();
    }

    bool contains(int key) const {
        return indices_.contains(key);
    }
    std::size_t node_count() const noexcept { return nodes_.size(); }
    std::size_t component_count() const noexcept { return components_; }
    bool empty() const noexcept { return nodes_.empty(); }

    void reserve(std::size_t expected_nodes) {
        check_capacity(expected_nodes);
        nodes_.reserve(expected_nodes);
        indices_.reserve(expected_nodes);
    }

    std::vector<std::vector<int>> groups() {
        std::vector<std::vector<int>> result;
        result.reserve(components_);
        std::vector<std::size_t> group_index(nodes_.size());
        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            if (nodes_[i].parent >= 0) continue;
            group_index[i] = result.size();
            result.emplace_back();
            result.back().reserve(static_cast<std::size_t>(-nodes_[i].parent));
        }
        for (std::size_t i = 0; i < nodes_.size(); ++i)
            result[group_index[root_index(static_cast<int>(i))]].push_back(nodes_[i].key);
        return result;
    }

    void clear() {
        indices_.clear();
        nodes_.clear();
        components_ = 0;
    }
};
#endif
