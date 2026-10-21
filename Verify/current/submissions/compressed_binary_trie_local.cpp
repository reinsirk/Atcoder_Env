// Verify: ローカル独立検査

// Library: DataStructure/Compressed_Binary_Trie.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/data-structure/binary-trie.hpp

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

// C++20以降。重複を許す非負整数の圧縮二分トライ。B = MAX_LOG + 1。
// Tはbool・const・volatileを除く整数型。MAX_LOGは最高位ビット番号。
// SKIP_RUNTIME_CHECKSで入力検査を除く。容量・オーバーフロー対策は常に有効。
// キー・XORマスクは 0 <= x < 2^B。NODESは未圧縮時の根込みノード上限。
// IDは重複可、-1は匿名。ID一覧は順序不定、次の更新または破棄まで有効。
#ifndef REINSIRK_COMPRESSED_BINARY_TRIE_DEFINED
#define REINSIRK_COMPRESSED_BINARY_TRIE_DEFINED
template <typename T, int MAX_LOG, std::size_t NODES = 16777216>
class CompressedBinaryTrie
{
    // CompressedBinaryTrie(): 空集合を構築する。コピー・ムーブは不可。
    // reserve(n): 異なる値n個分の圧縮ノード領域を上限内で事前確保。再配置時O(現在のノード数)。
    // size(): 重複込みの要素数をO(1)で返す。
    // empty(): 空集合ならtrueをO(1)で返す。
    // add(x,id=-1): xを1個追加する。id=-1は匿名。失敗時は値・件数・IDを保持。
    // insert_unique(x,id=-1): 未登録時だけ追加しtrue。登録済みならIDも変えずfalse。
    // del(x,id=-1): 1個削除してtrue。ID省略時は匿名優先、なければ任意のIDを削除。
    // find(x): {xの件数, ID一覧}。未登録は{0,空配列}。
    // min_element(): {最小値, ID一覧}。空でないこと。
    // max_element(): {最大値, ID一覧}。空でないこと。
    // get_kth(k): {重複込み昇順1始まりk番目の値, ID一覧}。1 <= k <= size()。
    // operate_xor(x): 全要素にXORをO(1)で適用する。
    // count(x): 匿名・重複を含むxの件数を返す。
    // contains(x): xが存在すればtrueを返す。
    // count_less(upper): upper未満の要素数。upper<=0なら0、範囲外ならsize()。
    // min_xor(x): {x^yの最小値, yのID一覧}。空でないこと。
    // max_xor(x): {x^yの最大値, yのID一覧}。空でないこと。
    // count_xor_less(x,upper): x^y<upperの件数。upperはcount_lessと同様。
    // 検索・削除はO(B)、追加は償却O(B)（再配置時O(ノード数)）。
    // 指定IDの削除はさらにその値のID数に比例する。
    static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>);
    static_assert(std::is_same_v<T, std::remove_cv_t<T>>);
    static_assert(MAX_LOG >= 0 && MAX_LOG < std::numeric_limits<T>::digits);
    static_assert(NODES > 0);
public:
    using Container = std::vector<int>;
    using size_type = std::size_t;
    using FindResult = std::pair<size_type, const Container &>;
    using ElementResult = std::pair<T, const Container &>;
private:
    using U = std::make_unsigned_t<T>;
    using Index = std::conditional_t<(NODES <= std::numeric_limits<std::uint32_t>::max()), std::uint32_t, size_type>;
    static constexpr Index nil = std::numeric_limits<Index>::max();
    static constexpr size_type kBits = MAX_LOG + 1;
    struct Node {
        size_type count = 0;
        Index child[2] = {nil, nil}; // 葉のchild[0]はID配列の添字。
        U key = 0; // 葉の値。内点では分岐より上位の共通接頭辞。
        int split = -1;
    };
    struct IdSlot {
        std::unique_ptr<Container> ids;
        Index next = nil;
    };
    std::vector<Node> nodes_;
    std::vector<IdSlot> id_slots_;
    Index root_ = nil, free_node_ = nil, free_id_ = nil;
    size_type free_count_ = 0, logical_nodes_ = 1;
    U lazy_ = 0;

    static U checked_key(T x) {
#ifndef SKIP_RUNTIME_CHECKS
        if constexpr (std::is_signed_v<T>)
            if (x < 0) throw std::out_of_range("CompressedBinaryTrie: negative key or XOR mask");
        if ((static_cast<U>(x) >> MAX_LOG) > U{1})
            throw std::out_of_range("CompressedBinaryTrie: key or XOR mask exceeds MAX_LOG");
#endif
        return static_cast<U>(x);
    }
    static unsigned bit(U x, int i) noexcept { return static_cast<unsigned>((x >> i) & U{1}); }
    static int high_bit(U x) noexcept { return static_cast<int>(std::bit_width(x)) - 1; }
    const Container &ids_of(Index n) const noexcept {
        static constexpr Container empty;
        return n == nil || nodes_[n].child[0] == nil ? empty : *id_slots_[nodes_[n].child[0]].ids;
    }
    Index leaf_for(U key) const noexcept {
        Index p = root_;
        while (p != nil && nodes_[p].split >= 0)
            p = nodes_[p].child[bit(key, nodes_[p].split)];
        return p;
    }
    Index find_leaf(U key) const noexcept {
        const auto p = leaf_for(key);
        return p != nil && nodes_[p].key == key ? p : nil;
    }
    // 確保を先に完了し、以後のノード操作は例外を送出しない。
    void reserve_nodes(size_type amount) {
        if (amount <= free_count_) return;
        const auto additional = amount - free_count_;
        const auto limit = std::min(NODES, static_cast<size_type>(nil));
        if (additional > limit || nodes_.size() > limit - additional)
            throw std::length_error("CompressedBinaryTrie: node index overflow");
        const auto required = nodes_.size() + additional;
        if (required > nodes_.capacity()) {
            const auto cap = nodes_.capacity();
            const auto grown = cap > limit / 2 ? limit : std::max<size_type>(8, cap * 2);
            nodes_.reserve(std::min(limit, std::max(required, grown)));
        }
    }
    Index acquire_node() noexcept {
        if (free_node_ != nil) {
            const Index p = free_node_;
            free_node_ = nodes_[p].child[0];
            --free_count_;
            nodes_[p] = Node{};
            return p;
        }
        const auto p = static_cast<Index>(nodes_.size());
        nodes_.emplace_back();
        return p;
    }
    void release_node(Index p) noexcept {
        nodes_[p].child[0] = free_node_;
        free_node_ = p;
        ++free_count_;
    }
    void reserve_id() {
        if (free_id_ != nil) return;
        const auto limit = static_cast<size_type>(nil);
        if (id_slots_.size() == limit) throw std::length_error("CompressedBinaryTrie: ID index overflow");
        if (id_slots_.size() == id_slots_.capacity()) {
            const auto cap = id_slots_.capacity();
            const auto grown = cap > limit / 2 ? limit : std::max<size_type>(4, cap * 2);
            id_slots_.reserve(std::min(limit, grown));
        }
    }
    Index acquire_id(std::unique_ptr<Container> ids) noexcept {
        Index p;
        if (free_id_ != nil) {
            p = free_id_;
            free_id_ = id_slots_[p].next;
        } else {
            p = static_cast<Index>(id_slots_.size());
            id_slots_.emplace_back();
        }
        id_slots_[p].ids = std::move(ids);
        return p;
    }
    void release_id(Index p) noexcept {
        id_slots_[p].ids.reset();
        id_slots_[p].next = free_id_;
        free_id_ = p;
    }
    bool insert(U key, int id, bool unique) {
        std::array<Index, kBits> path;
        size_type depth = 0;
        Index leaf = root_;
        while (leaf != nil && nodes_[leaf].split >= 0) {
            path[depth++] = leaf;
            leaf = nodes_[leaf].child[bit(key, nodes_[leaf].split)];
        }
        const bool duplicate = leaf != nil && nodes_[leaf].key == key;
        if (duplicate && unique) return false;
        if (size() == std::numeric_limits<size_type>::max())
            throw std::overflow_error("CompressedBinaryTrie: element count overflow");
        if (duplicate) {
            if (id != -1) {
                const Index ids = nodes_[leaf].child[0];
                if (ids != nil) id_slots_[ids].ids->push_back(id);
                else {
                    auto owned = std::make_unique<Container>(1, id);
                    reserve_id();
                    nodes_[leaf].child[0] = acquire_id(std::move(owned));
                }
            }
            ++nodes_[leaf].count;
            for (size_type i = 0; i < depth; ++i) ++nodes_[path[i]].count;
            return true;
        }
        const int split = leaf == nil ? MAX_LOG : high_bit(key ^ nodes_[leaf].key);
        const size_type increment = leaf == nil ? kBits : static_cast<size_type>(split + 1);
        if (increment > NODES - logical_nodes_)
            throw std::length_error("CompressedBinaryTrie: node limit exceeded");
        std::unique_ptr<Container> owned;
        if (id != -1) owned = std::make_unique<Container>(1, id);
        reserve_nodes(leaf == nil ? 1 : 2);
        if (owned) reserve_id();
        const Index fresh = acquire_node();
        nodes_[fresh].count = 1;
        nodes_[fresh].key = key;
        if (owned) nodes_[fresh].child[0] = acquire_id(std::move(owned));
        if (leaf == nil) root_ = fresh;
        else {
            size_type ancestors = 0;
            while (ancestors < depth && nodes_[path[ancestors]].split > split) ++ancestors;
            const Index old = ancestors == depth ? leaf : path[ancestors];
            const Index branch = acquire_node();
            nodes_[branch].split = split;
            nodes_[branch].key = key;
            nodes_[branch].count = nodes_[old].count + 1;
            const auto b = bit(key, split);
            nodes_[branch].child[b] = fresh;
            nodes_[branch].child[b ^ 1u] = old;
            if (ancestors == 0) root_ = branch;
            else {
                const Index parent = path[ancestors - 1];
                nodes_[parent].child[bit(key, nodes_[parent].split)] = branch;
                for (size_type i = 0; i < ancestors; ++i) ++nodes_[path[i]].count;
            }
        }
        logical_nodes_ += increment;
        return true;
    }
    ElementResult xor_extreme(U mask, bool maximum) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (empty()) throw std::out_of_range("CompressedBinaryTrie: min/max of an empty trie");
#endif
        Index p = root_;
        while (nodes_[p].split >= 0)
            p = nodes_[p].child[bit(mask, nodes_[p].split) ^ static_cast<unsigned>(maximum)];
        return {static_cast<T>(nodes_[p].key ^ mask), ids_of(p)};
    }
    template <class Bound>
    size_type xor_count_less(U mask, Bound upper) const noexcept {
        static_assert(std::is_integral_v<Bound>);
        if constexpr (std::is_signed_v<Bound>) if (upper <= 0) return 0;
        const auto limit = static_cast<std::uintmax_t>(upper);
        if (limit == 0) return 0;
        if ((limit >> MAX_LOG) > std::uintmax_t{1}) return size();
        const U bound = static_cast<U>(limit);
        Index p = root_;
        size_type result = 0;
        while (p != nil) {
            const Node &node = nodes_[p];
            const U value = node.key ^ mask;
            if (node.split < 0 || high_bit(value ^ bound) > node.split)
                return result + (value < bound ? node.count : 0);
            const unsigned zero = bit(mask, node.split);
            if (bit(bound, node.split)) {
                result += nodes_[node.child[zero]].count;
                p = node.child[zero ^ 1u];
            } else p = node.child[zero];
        }
        return result;
    }
public:
    CompressedBinaryTrie() = default;
    CompressedBinaryTrie(const CompressedBinaryTrie &) = delete;
    CompressedBinaryTrie &operator=(const CompressedBinaryTrie &) = delete;
    CompressedBinaryTrie(CompressedBinaryTrie &&) = delete;
    CompressedBinaryTrie &operator=(CompressedBinaryTrie &&) = delete;
    void reserve(size_type distinct) {
        if (distinct == 0) return;
        const auto limit = std::min(NODES, static_cast<size_type>(nil));
        const auto required = distinct > limit / 2 ? limit : distinct * 2 - 1;
        if (required > nodes_.capacity()) nodes_.reserve(required);
    }
    [[nodiscard]] size_type size() const noexcept { return root_ == nil ? 0 : nodes_[root_].count; }
    [[nodiscard]] bool empty() const noexcept { return root_ == nil; }
    void add(T x, int id = -1) { insert(checked_key(x) ^ lazy_, id, false); }
    bool insert_unique(T x, int id = -1) { return insert(checked_key(x) ^ lazy_, id, true); }
    bool del(T x, int id = -1) {
        const U key = checked_key(x) ^ lazy_;
        std::array<Index, kBits> path;
        size_type depth = 0;
        Index p = root_;
        while (p != nil && nodes_[p].split >= 0) {
            path[depth++] = p;
            p = nodes_[p].child[bit(key, nodes_[p].split)];
        }
        if (p == nil || nodes_[p].key != key) return false;
        const Index ids = nodes_[p].child[0];
        if (id != -1) {
            if (ids == nil) return false;
            auto &values = *id_slots_[ids].ids;
            const auto it = std::find(values.begin(), values.end(), id);
            if (it == values.end()) return false;
            *it = values.back();
            values.pop_back();
        } else if (ids != nil && nodes_[p].count == id_slots_[ids].ids->size())
            id_slots_[ids].ids->pop_back();
        if (--nodes_[p].count != 0) {
            for (size_type i = 0; i < depth; ++i) --nodes_[path[i]].count;
            return true;
        }
        if (ids != nil) release_id(ids);
        if (depth == 0) {
            root_ = nil;
            logical_nodes_ -= kBits;
        } else {
            const Index parent = path[depth - 1];
            const Index sibling = nodes_[parent].child[bit(key, nodes_[parent].split) ^ 1u];
            if (depth == 1) root_ = sibling;
            else {
                const Index grandparent = path[depth - 2];
                nodes_[grandparent].child[bit(key, nodes_[grandparent].split)] = sibling;
                for (size_type i = 0; i + 1 < depth; ++i) --nodes_[path[i]].count;
            }
            logical_nodes_ -= static_cast<size_type>(nodes_[parent].split + 1);
            release_node(parent);
        }
        release_node(p);
        return true;
    }
    [[nodiscard]] FindResult find(T x) const {
        const auto p = find_leaf(checked_key(x) ^ lazy_);
        return {p == nil ? 0 : nodes_[p].count, ids_of(p)};
    }
    [[nodiscard]] ElementResult min_element() const { return xor_extreme(lazy_, false); }
    [[nodiscard]] ElementResult max_element() const { return xor_extreme(lazy_, true); }
    [[nodiscard]] ElementResult get_kth(std::int64_t k) const {
#ifndef SKIP_RUNTIME_CHECKS
        if (k <= 0 || static_cast<std::uint64_t>(k) > size())
            throw std::out_of_range("CompressedBinaryTrie: k must satisfy 1 <= k <= size()");
#endif
        auto rank = static_cast<size_type>(k);
        Index p = root_;
        while (nodes_[p].split >= 0) {
            const auto zero = bit(lazy_, nodes_[p].split);
            const auto left = nodes_[p].child[zero];
            if (rank <= nodes_[left].count) p = left;
            else { rank -= nodes_[left].count; p = nodes_[p].child[zero ^ 1u]; }
        }
        return {static_cast<T>(nodes_[p].key ^ lazy_), ids_of(p)};
    }
    void operate_xor(T x) { lazy_ ^= checked_key(x); }
    [[nodiscard]] size_type count(T x) const {
        const auto p = find_leaf(checked_key(x) ^ lazy_);
        return p == nil ? 0 : nodes_[p].count;
    }
    [[nodiscard]] bool contains(T x) const { return find_leaf(checked_key(x) ^ lazy_) != nil; }
    template <class Bound> [[nodiscard]] size_type count_less(Bound upper) const noexcept {
        return xor_count_less(lazy_, upper);
    }
    [[nodiscard]] ElementResult min_xor(T x) const { return xor_extreme(checked_key(x) ^ lazy_, false); }
    [[nodiscard]] ElementResult max_xor(T x) const { return xor_extreme(checked_key(x) ^ lazy_, true); }
    template <class Bound> [[nodiscard]] size_type count_xor_less(T x, Bound upper) const {
        return xor_count_less(checked_key(x) ^ lazy_, upper);
    }
};
#endif


// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>

int main() {
    const auto require=[](bool ok) { if(!ok)throw std::runtime_error("trie reference mismatch"); };
    std::mt19937 random(57623);
    CompressedBinaryTrie<std::uint16_t,7> trie;std::multiset<std::uint16_t> model;
    for(int step=0;step<5000;++step) {
        const std::uint16_t x=random()%256;
        switch(random()%4) {
        case 0: case 1:trie.add(x);model.insert(x);break;
        case 2: {
            const auto it=model.find(x);const bool found=it!=model.end();require(trie.del(x)==found);
            if(found)model.erase(it);break;
        }
        default: {
            trie.operate_xor(x);std::multiset<std::uint16_t> next;
            for(auto y:model)next.insert(y^x);model.swap(next);break;
        }
        }
        require(trie.size()==model.size() && trie.empty()==model.empty());
        require(trie.count(x)==model.count(x) && trie.contains(x)==model.contains(x));
        require(trie.find(x).first==model.count(x));
        const int upper=static_cast<int>(random()%260)-2;
        require(trie.count_less(upper)==static_cast<std::size_t>(std::count_if(model.begin(),model.end(),[&](auto y){return y<upper;})));
        require(trie.count_xor_less(x,upper)==static_cast<std::size_t>(std::count_if(model.begin(),model.end(),[&](auto y){return (x^y)<upper;})));
        if(!model.empty()) {
            require(trie.min_element().first==*model.begin());require(trie.max_element().first==*model.rbegin());
            const std::size_t k=random()%model.size();require(trie.get_kth(k+1).first==*std::next(model.begin(),k));
            std::uint16_t lo=255,hi=0;for(auto y:model) {lo=std::min<std::uint16_t>(lo,x^y);hi=std::max<std::uint16_t>(hi,x^y);}
            require(trie.min_xor(x).first==lo && trie.max_xor(x).first==hi);
        }
    }
    CompressedBinaryTrie<std::uint32_t,31> ids;ids.add(7,10);ids.add(7,10);ids.add(7,20);ids.add(7);
    require(ids.count(7)==4 && ids.find(7).second.size()==3);
    require(ids.del(7,10) && ids.count(7)==3 && ids.find(7).second.size()==2);
    ids.operate_xor(0xffffffffU);require(ids.contains(0xfffffff8U));
    require(ids.count_less(std::uint64_t{1}<<32)==3);
    require(ids.count_xor_less(0xffffffffU,std::uint64_t{1}<<32)==3);
    CompressedBinaryTrie<std::uint32_t, 29> unique;
    unique.reserve(8);
    require(unique.insert_unique(9, 42));
    require(!unique.insert_unique(9, 99));
    require(unique.size() == 1 && unique.find(9).second == std::vector<int>{42});
    std::cout<<"OK\n";
}
