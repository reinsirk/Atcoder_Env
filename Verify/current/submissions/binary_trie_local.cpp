// Verify: ローカル独立検査

// Library: DataStructure/Binary_Trie.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/data-structure/binary-trie.hpp
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/binary_trie.md
//
// API一覧
// [BinaryTrie<T,MAX_LOG,NODES>]
// C++20以降。重複を許す非負整数の二分トライ。T は bool / const / volatile を除く整数型。
// キーと XOR マスクは 0 <= x < 2^(MAX_LOG + 1)。MAX_LOG は最高位ビット番号。
// MAX_LOGの既定値はstd::numeric_limits<T>::digits - 1。第2テンプレート引数で手動指定も可。
// NODES は根込みのノード上限。以下 B = MAX_LOG + 1。
// SKIP_RUNTIME_CHECKS を定義すると入力検査を省く。容量保護は常に有効。
// get_kth は重複込み・1始まり。ID は重複可、-1 は匿名（ID一覧には含めない）。
// ID省略の削除は匿名を優先し、なければ任意のID付き要素を削除する。
// ID一覧は順序不定の読み取り専用参照。次の更新または破棄まで有効。
// BinaryTrie(): 空の集合を構築する。
// size(): 重複・匿名要素を含む要素数を返す（O(1)）。
// empty(): 空ならtrueを返す（O(1)）。
// add(x, id=-1): xを1個追加する。id=-1は匿名。
// insert_unique(x,id=-1): 未登録時だけ追加してtrue。登録済みならIDも変えずfalse。
// del(x, id=-1): xを1個削除し、削除できればtrue。id指定時はそのIDを削除する。
// find(x): {xの個数, ID一覧}を返す。未登録なら{0, 空配列}。
// min_element(): {最小値, ID一覧}を返す。空でないこと。
// max_element(): {最大値, ID一覧}を返す。空でないこと。
// get_kth(k): {昇順でk番目の値, ID一覧}を返す。重複込み・1始まり。
// operate_xor(x): 全要素の値にXORを適用する（O(1)）。
// count(x): xの個数を返す。匿名要素・重複も数える。
// contains(x): xが1個以上あればtrueを返す。
// count_less(upper): upper未満の要素数を重複込みで返す。
// min_xor(x): {x ^ yの最小値, 対応するyのID一覧}を返す。空でないこと。
// max_xor(x): {x ^ yの最大値, 対応するyのID一覧}を返す。空でないこと。
// count_xor_less(x, upper): x ^ y < upperとなる要素数を重複込みで返す。
// 値の検索・順位・XORクエリはO(B)。整数型のupperが0以下なら個数は0。
// 構築時に最初のノードブロックを確保する。コピー・ムーブは不可。
// add/insert_uniqueは償却O(B)、ID追加は償却O(1)。確保失敗時は値・件数・IDを保持する。
// delはID省略ならO(B)、指定IDの削除はO(B+その値のID数)。値またはIDがなければfalse。
// get_kthは1<=k<=size()が前提。operate_xor後にadd(y)する値はy。
// countはID一覧を参照しない。upperはキー型より広い整数型も可、キー範囲を超えればsize()。
// min_xor/max_xor/count_xor_lessは状態を変更しない。返す値は保存値yではなくx^y。

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef SIRKIT_BINARY_TRIE_DEFINED
#define SIRKIT_BINARY_TRIE_DEFINED
template <typename T, int MAX_LOG = std::numeric_limits<T>::digits - 1,
          std::size_t NODES = 16777216>
class BinaryTrie
{
    static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>,
                  "T must be an integer type other than bool");
    static_assert(std::is_same_v<T, std::remove_cv_t<T>>,
                  "T must not be const or volatile");
    static_assert(MAX_LOG >= 0 && MAX_LOG < std::numeric_limits<T>::digits,
                  "MAX_LOG is outside the nonnegative range of T");
    static_assert(NODES > 0, "NODES must include at least the root");

public:
    using Container = std::vector<int>;
    using size_type = std::size_t;
    using FindResult = std::pair<size_type, const Container &>;
    using ElementResult = std::pair<T, const Container &>;

private:
    using U = std::make_unsigned_t<T>;
    static constexpr size_type kBits = MAX_LOG + 1;
    static constexpr size_type kBlockSize = 8192;
    static constexpr size_type kInitialNodes = std::min(size_type{4096}, NODES);

    // 内部では子、葉ではID枠への非所有参照を保持する。
    struct Node
    {
        struct IdData { Container *ptr; size_type slot; }; // ptrはID枠内への非所有ポインタ。
        union Data
        {
            Node *child[2]; // ノードブロック内への非所有ポインタ。
            IdData ids;
            Data() noexcept : child{nullptr, nullptr} {}
        } data;
        size_type count = 0;
    };
    // 初4096ノードは従来の配置を保ち、以降のノードを小型化する。
    struct InitialNode : Node { size_type padding = 0; };
    struct InitialBlock { size_type padding = 0; std::array<InitialNode, kInitialNodes> nodes; };

    // ID配列本体もブロックで所有し、拡張時の参照を維持する。
    struct IdSlot
    {
        Container ids;
        size_type next = std::numeric_limits<size_type>::max();
    };
    static constexpr size_type kNoSlot = std::numeric_limits<size_type>::max();
    static constexpr size_type kInitialIdSlots = std::min(size_type{32}, NODES / 2);
    static constexpr size_type kIdBlockSize = 4096;

    IdSlot &id_slot(size_type i) noexcept
    {
        if (i < kInitialIdSlots) return initial_ids_[i];
        const auto index = i - kInitialIdSlots;
        return id_blocks_[index / kIdBlockSize][index % kIdBlockSize];
    }
    void append_id(Node *leaf, int id)
    {
        const bool recycled = free_id_slot_ != kNoSlot;
        const auto index = recycled ? free_id_slot_ : next_id_;
        if (!recycled && next_id_ == id_capacity_)
        {
            if (id_capacity_ == NODES)
                throw std::length_error("BinaryTrie: ID slot limit exceeded");
            const auto n = std::min(kIdBlockSize, NODES - id_capacity_);
            id_blocks_.push_back(std::make_unique<IdSlot[]>(n));
            id_capacity_ += n;
        }
        auto &slot = id_slot(index);
        slot.ids.push_back(id);
        if (recycled) free_id_slot_ = slot.next;
        else ++next_id_;
        leaf->data.ids = {&slot.ids, index};
    }

    // ノードをブロック単位で所有し、拡張時もアドレスを維持する。
    std::vector<std::unique_ptr<Node[]>> blocks_;
    size_type capacity_ = 0;
    size_type next_unused_ = 0;
    Node *free_list_ = nullptr;
    Node *root_ = nullptr;
    U lazy_ = 0;
    std::unique_ptr<InitialBlock> initial_;
    std::array<IdSlot, kInitialIdSlots> initial_ids_;
    std::vector<std::unique_ptr<IdSlot[]>> id_blocks_;
    size_type id_capacity_ = kInitialIdSlots, next_id_ = 0;
    size_type free_id_slot_ = kNoSlot;

    // 再利用リストか未使用領域から取得し、必要なら上限内でブロックを確保する。
    Node *acquire_node()
    {
        if (free_list_)
        {
            Node *node = free_list_;
            free_list_ = node->data.child[0];
            node->data.child[0] = node->data.child[1] = nullptr;
            return node;
        }
        if (next_unused_ == capacity_)
        {
            if (capacity_ == NODES)
            {
                throw std::length_error("BinaryTrie: node limit exceeded");
            }
            const auto n = std::min(kBlockSize, NODES - capacity_);
            blocks_.push_back(std::make_unique<Node[]>(n));
            capacity_ += n;
        }
        const auto index = next_unused_++;
        if (index < kInitialNodes) return &initial_->nodes[index];
        const auto offset = index - kInitialNodes;
        return &blocks_[offset / kBlockSize + 1][offset % kBlockSize];
    }

#if defined(__GNUC__) && !defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
    template<class N>
    static void link_chunk(N *nodes, size_type count, U key,
                           std::array<Node *, kBits + 1> &path, size_type &depth,
                           Node *&previous, size_type &last_new) noexcept
    {
        if (previous)
            previous->data.child[bit(key, MAX_LOG - static_cast<int>(depth - 1))] = nodes;
        if (count > 1)
        {
            U remaining = static_cast<U>(key << (std::numeric_limits<U>::digits - kBits + depth));
            for (size_type j = 0; j + 1 < count; ++j)
            {
                path[depth] = nodes + j;
                const auto b = static_cast<unsigned>(remaining >> (std::numeric_limits<U>::digits - 1));
                nodes[j].data.child[b] = nodes + j + 1;
                remaining = static_cast<U>(remaining << 1);
                ++depth;
            }
        }
        path[depth++] = nodes + count - 1;
        previous = nodes + count - 1;
        last_new = depth - 1;
    }
    void acquire_suffix(U key, std::array<Node *, kBits + 1> &path,
                        size_type first, size_type &last_new)
    {
        size_type depth = first;
        Node *previous = nullptr;
        while (depth <= kBits && free_list_)
        {
            Node *node = free_list_;
            free_list_ = node->data.child[0];
            node->data.child[0] = node->data.child[1] = nullptr;
            if (previous)
                previous->data.child[bit(key, MAX_LOG - static_cast<int>(depth - 1))] = node;
            path[depth] = node;
            previous = node;
            last_new = depth++;
        }
        while (depth <= kBits)
        {
            if (next_unused_ == capacity_)
            {
                if (capacity_ == NODES) throw std::length_error("BinaryTrie: node limit exceeded");
                const auto n = std::min(kBlockSize, NODES - capacity_);
                blocks_.push_back(std::make_unique<Node[]>(n));
                capacity_ += n;
            }
            const auto count = std::min(kBits - depth + 1, capacity_ - next_unused_);
            const auto index = next_unused_;
            next_unused_ += count;
            if (index < kInitialNodes)
                link_chunk(&initial_->nodes[index], count, key, path, depth, previous, last_new);
            else
            {
                const auto offset = index - kInitialNodes;
                link_chunk(&blocks_[offset / kBlockSize + 1][offset % kBlockSize],
                           count, key, path, depth, previous, last_new);
            }
        }
    }

#endif

    // ノードを初期化して再利用リストへ戻す。子の回収は呼び出し側で行う。
    void release_node(Node *node, bool leaf) noexcept
    {
        if (leaf && node->data.ids.ptr)
        {
            const auto slot = node->data.ids.slot;
            auto &entry = id_slot(slot);
            Container{}.swap(entry.ids);
            entry.next = free_id_slot_;
            free_id_slot_ = slot;
        }
        node->count = 0;
        node->data.child[0] = free_list_;
        node->data.child[1] = nullptr;
        free_list_ = node;
    }

    // 値の範囲を検査し、内部用の符号なし型へ変換する。
    static U checked_key(T x)
    {
#ifndef SKIP_RUNTIME_CHECKS
        if constexpr (std::is_signed_v<T>)
        {
            if (x < 0)
            {
                throw std::out_of_range("BinaryTrie: negative key or XOR mask");
            }
        }
        const U u = static_cast<U>(x);
        // 型幅以上のシフトを避ける。
        if ((u >> MAX_LOG) > U{1})
        {
            throw std::out_of_range("BinaryTrie: key or XOR mask exceeds MAX_LOG");
        }
        return u;
#else
        return static_cast<U>(x);
#endif
    }

    // 下から i 番目のビットを取り出す（0 <= i <= MAX_LOG）。
    static unsigned bit(U x, int i) noexcept
    {
        return static_cast<unsigned>((x >> i) & U{1});
    }

    // 読み取り専用のID一覧を返す。ノードやIDがなければ空配列。
    static const Container &ids_of(const Node *node) noexcept
    {
        static constexpr Container empty;
        return node && node->data.ids.ptr ? *node->data.ids.ptr : empty;
    }

    // 内部キー（公開キー ^ lazy_）の葉を探す。未登録なら nullptr。
    const Node *find_leaf(U stored_key) const noexcept
    {
        const Node *node = root_;
        for (int i = MAX_LOG; i >= 0 && node; --i)
        {
            node = node->data.child[bit(stored_key, i)];
        }
        return node;
    }

    // XOR反映後の最小値・最大値を選ぶ共通処理（O(B)）。
    ElementResult extreme(bool maximum) const
    {
#ifndef SKIP_RUNTIME_CHECKS
        if (empty())
        {
            throw std::out_of_range("BinaryTrie: min/max of an empty trie");
        }
#endif
        const Node *node = root_;
        U value = 0;
        for (int i = MAX_LOG; i >= 0; --i)
        {
            const unsigned logical_bit = static_cast<unsigned>(maximum);
            const unsigned preferred = bit(lazy_, i) ^ logical_bit;
#if defined(__clang__)
            const Node *next = node->data.child[preferred];
            const bool missing = next == nullptr;
            value |= static_cast<U>(missing ^ maximum) << i;
            node = next ? next : node->data.child[preferred ^ 1u];
#else
            const bool available = node->data.child[preferred] != nullptr;
            const unsigned chosen = available ? preferred : (preferred ^ 1u);
            if ((chosen ^ bit(lazy_, i)) != 0)
                value |= U{1} << i;
            node = node->data.child[chosen];
#endif
        }
        return {static_cast<T>(value), ids_of(node)};
    }

    // lazy_と問い合わせ用XORをまとめて、XOR値の最小・最大を求める。
    ElementResult xor_extreme(U mask, bool maximum) const
    {
#ifndef SKIP_RUNTIME_CHECKS
        if (empty())
            throw std::out_of_range("BinaryTrie: min/max XOR of an empty trie");
#endif
        const Node *node = root_;
        U value = 0;
        for (int i = MAX_LOG; i >= 0; --i)
        {
            const unsigned zero = bit(mask, i);
            const unsigned preferred = zero ^ static_cast<unsigned>(maximum);
            const unsigned chosen = node->data.child[preferred] ? preferred : (preferred ^ 1u);
#if defined(__clang__)
            value = (value << 1) | static_cast<U>(chosen ^ zero);
#else
            if ((chosen ^ zero) != 0)
                value |= U{1} << i;
#endif
            node = node->data.child[chosen];
        }
        return {static_cast<T>(value), ids_of(node)};
    }

    // XOR後の値が上限未満となる個数。型幅以上のシフトを使わず上限を扱う。
    template <class Bound>
    size_type xor_count_less(U mask, Bound upper) const noexcept
    {
        static_assert(std::is_integral_v<Bound>, "upper must be an integer");
        if constexpr (std::is_signed_v<Bound>)
        {
            if (upper <= 0)
                return 0;
        }
        const auto limit = static_cast<std::uintmax_t>(upper);
        if (limit == 0)
            return 0;
        if ((limit >> MAX_LOG) > std::uintmax_t{1})
            return size();
        const U bound = static_cast<U>(limit);
        const Node *node = root_;
        size_type result = 0;
        for (int i = MAX_LOG; i >= 0 && node; --i)
        {
            const unsigned zero = bit(mask, i);
            if (bit(bound, i) != 0)
            {
                if (const Node *left = node->data.child[zero])
                    result += left->count;
                node = node->data.child[zero ^ 1u];
            }
            else
            {
                node = node->data.child[zero];
            }
        }
        return result;
    }

public:
    BinaryTrie()
    {
        initial_ = std::make_unique_for_overwrite<InitialBlock>();
        blocks_.push_back(nullptr);
        capacity_ = kInitialNodes;
        root_ = acquire_node();
    }

    BinaryTrie(const BinaryTrie &) = delete;
    BinaryTrie &operator=(const BinaryTrie &) = delete;
    BinaryTrie(BinaryTrie &&) = delete;
    BinaryTrie &operator=(BinaryTrie &&) = delete;

    [[nodiscard]] size_type size() const noexcept { return root_->count; }
    [[nodiscard]] bool empty() const noexcept { return size() == 0; }

    void add(T x, int id = -1)
    {
        const U key = checked_key(x) ^ lazy_;
        if (size() == std::numeric_limits<size_type>::max())
        {
            throw std::overflow_error("BinaryTrie: element count overflow");
        }
        std::array<Node *, kBits + 1> path;
        path[0] = root_;
        size_type first_new = 0;
        size_type last_new = 0;
        try
        {
            for (size_type depth = 1; depth <= kBits; ++depth)
            {
                const unsigned b = bit(key, MAX_LOG - static_cast<int>(depth - 1));
                Node *child = path[depth - 1]->data.child[b];
                if (!child)
                {
#if defined(__GNUC__) && !defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
                    first_new = depth;
                    acquire_suffix(key, path, depth, last_new);
                    break;
#else
                    child = acquire_node();
                    if (!first_new)
                    {
                        first_new = depth; // 成功するまで新しい部分木を接続しない。
                    }
                    else
                    {
                        path[depth - 1]->data.child[b] = child;
                    }
                    last_new = depth;
#endif
                }
                path[depth] = child;
            }
            Node *leaf = path[kBits];
            if (first_new) leaf->data.ids.ptr = nullptr;
            if (id != -1)
            {
                if (leaf->data.ids.ptr) leaf->data.ids.ptr->push_back(id);
                else append_id(leaf, id);
            }
        }
        catch (...)
        {
            if (first_new)
            {
                for (size_type depth = last_new; depth >= first_new; --depth)
                {
                    release_node(path[depth], depth == kBits);
                }
            }
            throw;
        }
        if (first_new)
        {
            const auto b = bit(key, MAX_LOG - static_cast<int>(first_new - 1));
            path[first_new - 1]->data.child[b] = path[first_new];
        }
#if defined(__GNUC__) && !defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
        size_type depth = 0;
        for (; depth + 4 <= kBits + 1; depth += 4)
        {
            ++path[depth + 0]->count;
            ++path[depth + 1]->count;
            ++path[depth + 2]->count;
            ++path[depth + 3]->count;
        }
        for (; depth <= kBits; ++depth)
            ++path[depth]->count;
#else
        for (Node *node : path)
            ++node->count;
#endif
    }

    bool insert_unique(T x, int id = -1)
    {
        const U key = checked_key(x) ^ lazy_;
        if (size() == std::numeric_limits<size_type>::max())
        {
            if (find_leaf(key)) return false;
            throw std::overflow_error("BinaryTrie: element count overflow");
        }
        std::array<Node *, kBits + 1> path;
        path[0] = root_;
        size_type first_new = 0;
        size_type last_new = 0;
        try
        {
            for (size_type depth = 1; depth <= kBits; ++depth)
            {
                const unsigned b = bit(key, MAX_LOG - static_cast<int>(depth - 1));
                Node *child = path[depth - 1]->data.child[b];
                if (!child)
                {
#if defined(__GNUC__) && !defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
                    first_new = depth;
                    acquire_suffix(key, path, depth, last_new);
                    break;
#else
                    child = acquire_node();
                    if (!first_new)
                    {
                        first_new = depth; // 成功するまで新しい部分木を接続しない。
                    }
                    else
                    {
                        path[depth - 1]->data.child[b] = child;
                    }
                    last_new = depth;
#endif
                }
                path[depth] = child;
            }
            if (!first_new) return false;
            Node *leaf = path[kBits];
            if (first_new) leaf->data.ids.ptr = nullptr;
            if (id != -1)
            {
                if (leaf->data.ids.ptr) leaf->data.ids.ptr->push_back(id);
                else append_id(leaf, id);
            }
        }
        catch (...)
        {
            if (first_new)
            {
                for (size_type depth = last_new; depth >= first_new; --depth)
                {
                    release_node(path[depth], depth == kBits);
                }
            }
            throw;
        }
        if (first_new)
        {
            const auto b = bit(key, MAX_LOG - static_cast<int>(first_new - 1));
            path[first_new - 1]->data.child[b] = path[first_new];
        }
#if defined(__GNUC__) && !defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
        size_type depth = 0;
        for (; depth + 4 <= kBits + 1; depth += 4)
        {
            ++path[depth + 0]->count;
            ++path[depth + 1]->count;
            ++path[depth + 2]->count;
            ++path[depth + 3]->count;
        }
        for (; depth <= kBits; ++depth)
            ++path[depth]->count;
#else
        for (Node *node : path)
            ++node->count;
#endif
        return true;
    }

    bool del(T x, int id = -1)
    {
        const U key = checked_key(x) ^ lazy_;
        std::array<Node *, kBits + 1> path;
        path[0] = root_;
        for (size_type depth = 1; depth <= kBits; ++depth)
        {
            const auto b = bit(key, MAX_LOG - static_cast<int>(depth - 1));
            path[depth] = path[depth - 1]->data.child[b];
            if (!path[depth])
                return false;
        }
        Node *leaf = path[kBits];
        if (id != -1)
        {
            if (!leaf->data.ids.ptr)
                return false;
            auto &ids = *leaf->data.ids.ptr;
            const auto it = std::find(ids.begin(), ids.end(), id);
            if (it == ids.end())
                return false;
            *it = ids.back();
            ids.pop_back();
        }
        else if (leaf->data.ids.ptr && leaf->count == leaf->data.ids.ptr->size())
        {
            leaf->data.ids.ptr->pop_back();
        }
        for (Node *node : path)
            --node->count;
        for (size_type depth = kBits; depth > 0; --depth)
        {
            if (path[depth]->count != 0)
                break;
            const auto b = bit(key, MAX_LOG - static_cast<int>(depth - 1));
            path[depth - 1]->data.child[b] = nullptr;
            release_node(path[depth], depth == kBits);
        }
        return true;
    }

    [[nodiscard]] FindResult find(T x) const
    {
        const Node *leaf = find_leaf(checked_key(x) ^ lazy_);
        return {leaf ? leaf->count : 0, ids_of(leaf)};
    }

    [[nodiscard]] ElementResult min_element() const { return extreme(false); }

    [[nodiscard]] ElementResult max_element() const { return extreme(true); }

    [[nodiscard]] ElementResult get_kth(std::int64_t k) const
    {
#ifndef SKIP_RUNTIME_CHECKS
        if (k <= 0 || static_cast<std::uint64_t>(k) > size())
        {
            throw std::out_of_range("BinaryTrie: k must satisfy 1 <= k <= size()");
        }
#endif
        auto rank = static_cast<size_type>(k);
        const Node *node = root_;
        U value = 0;
        for (int i = MAX_LOG; i >= 0; --i)
        {
            const auto zero = bit(lazy_, i);
#if defined(__GNUC__) && !defined(__clang__) && defined(SKIP_RUNTIME_CHECKS)
            if constexpr (MAX_LOG >= 48)
            {
                const Node *left = node->data.child[zero];
                const size_type count = left ? left->count : 0;
                if (rank < count) node = left;
                else if (rank == count) {
                    // 左部分木の末尾と確定したら、残りは最大値をたどる。
                    node = left;
                    for (--i; i >= 0; --i) {
                        const auto z = bit(lazy_, i);
                        if (const Node *right = node->data.child[z ^ 1u]) {
                            value |= U{1} << i; node = right;
                        } else node = node->data.child[z];
                    }
                    return {static_cast<T>(value), ids_of(node)};
                } else {
                    rank -= count;
                    value |= U{1} << i;
                    node = node->data.child[zero ^ 1u];
                }
            }
            else
#endif
            {
                const Node *left = node->data.child[zero];
                const size_type count = left ? left->count : 0;
                if (rank <= count)
                {
                    node = left;
                }
                else
                {
                    rank -= count;
                    value |= U{1} << i;
                    node = node->data.child[zero ^ 1u];
                }
            }
        }
        return {static_cast<T>(value), ids_of(node)};
    }

    void operate_xor(T x) { lazy_ ^= checked_key(x); }

    [[nodiscard]] size_type count(T x) const
    {
        const Node *leaf = find_leaf(checked_key(x) ^ lazy_);
        return leaf ? leaf->count : 0;
    }

    [[nodiscard]] bool contains(T x) const
    {
        return find_leaf(checked_key(x) ^ lazy_) != nullptr;
    }

    template <class Bound>
    [[nodiscard]] size_type count_less(Bound upper) const noexcept
    {
        return xor_count_less(lazy_, upper);
    }

    [[nodiscard]] ElementResult min_xor(T x) const
    {
        return xor_extreme(checked_key(x) ^ lazy_, false);
    }

    [[nodiscard]] ElementResult max_xor(T x) const
    {
        return xor_extreme(checked_key(x) ^ lazy_, true);
    }

    template <class Bound>
    [[nodiscard]] size_type count_xor_less(T x, Bound upper) const
    {
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
    BinaryTrie<std::uint16_t,7> trie;std::multiset<std::uint16_t> model;
    for(int step=0;step<5000;++step) {
        const std::uint16_t x=random()%256;
        switch(random()%5) {
        case 0: case 1:trie.add(x);model.insert(x);break;
        case 2: {
            const auto it=model.find(x);const bool found=it!=model.end();require(trie.del(x)==found);
            if(found)model.erase(it);break;
        }
        case 3: {
            trie.operate_xor(x);std::multiset<std::uint16_t> next;
            for(auto y:model)next.insert(y^x);model.swap(next);break;
        }
        default: {
            const bool absent=!model.contains(x);
            require(trie.insert_unique(x)==absent);
            if(absent)model.insert(x);break;
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
    BinaryTrie<std::uint32_t,31> ids;ids.add(7,10);ids.add(7,10);ids.add(7,20);ids.add(7);
    require(ids.count(7)==4 && ids.find(7).second.size()==3);
    require(ids.del(7,10) && ids.count(7)==3 && ids.find(7).second.size()==2);
    ids.operate_xor(0xffffffffU);require(ids.contains(0xfffffff8U));
    require(ids.count_less(std::uint64_t{1}<<32)==3);
    require(ids.count_xor_less(0xffffffffU,std::uint64_t{1}<<32)==3);
    require(!ids.insert_unique(0xfffffff8U,99) && ids.count(0xfffffff8U)==3);
    require(std::find(ids.find(0xfffffff8U).second.begin(),ids.find(0xfffffff8U).second.end(),99)==ids.find(0xfffffff8U).second.end());
    require(ids.insert_unique(42,99) && !ids.insert_unique(42));
    require(ids.find(42).second==std::vector<int>{99});
    std::cout<<"OK\n";
}
