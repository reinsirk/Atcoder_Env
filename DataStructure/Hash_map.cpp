// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/hashmap/hashmap.hpp
#include <algorithm>
#include <bit>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef REINSIRK_GENERIC_HASH_DEFINED
#define REINSIRK_GENERIC_HASH_DEFINED
struct GenericHash {
    // GenericHash(): 実行時に選んだ種で構築する。同じオブジェクトでは結果は一定。
    // GenericHash(seed): 指定した種で構築する。std::hash対応型の結果はその実装にも依存する。
    // operator()(x): 整数・列挙・浮動小数・tuple・range・std::hash対応型を再帰的にハッシュ化する。
    // unorderedコンテナの要素順は無視する。独自の同値関係には対応するHasherも用意する。
private:
    std::uint64_t seed_;
    static constexpr std::uint64_t mix(std::uint64_t x) noexcept {
        x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
        x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
        return x ^ (x >> 31);
    }
    template <class> static constexpr bool dependent_false = false;
    template <class T> static constexpr bool integral_pair = requires (const T& value) {
        value.first; value.second;
        requires std::is_integral_v<std::remove_cvref_t<decltype(value.first)>>;
        requires std::is_integral_v<std::remove_cvref_t<decltype(value.second)>>;
    };
    template <class T>
    std::uint64_t raw(const T& value) const {
        using U = std::remove_cvref_t<T>;
        if constexpr (std::is_integral_v<U>) {
            static_assert(sizeof(U) <= sizeof(std::uint64_t), "GenericHash: integer exceeds 64 bits");
            return static_cast<std::uint64_t>(value);
        } else if constexpr (std::is_enum_v<U>) {
            return static_cast<std::uint64_t>(static_cast<std::underlying_type_t<U>>(value));
        } else if constexpr (requires { typename std::tuple_size<U>::type; }) {
            std::uint64_t result = seed_;
            std::apply([&](const auto&... elements) {
                ((result = result * UINT64_C(11995408973635179863) + raw(elements)), ...);
            }, value);
            return mix(result ^ std::tuple_size_v<U>);
        } else if constexpr (requires { typename U::hasher; typename U::key_equal; std::begin(value); std::end(value); }) {
            // 同じ要素集合は列挙順にかかわらず同じ値になる。
            std::uint64_t sum = 0, second = 0, count = 0;
            for (const auto& element : value) {
                const auto h = mix(raw(element) + seed_);
                sum += h;
                second += h * h;
                ++count;
            }
            return mix(sum ^ std::rotl(second, 23) ^ count);
        } else if constexpr (requires { { std::hash<U>{}(value) } -> std::convertible_to<std::size_t>; }) {
            // 浮動小数の+0/-0もstd::hashに従い等しい値にする。
            return static_cast<std::uint64_t>(std::hash<U>{}(value));
        } else if constexpr (requires { std::begin(value); std::end(value); }) {
            std::uint64_t result = seed_, count = 0;
            for (const auto& element : value) {
                result = result * UINT64_C(11995408973635179863) + raw(element);
                ++count;
            }
            return mix(result ^ count);
        } else {
            static_assert(dependent_false<U>, "GenericHash: provide std::hash or a custom Hasher for this key");
        }
    }
public:
    GenericHash() : GenericHash(static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count())) {}
    explicit constexpr GenericHash(std::uint64_t seed) noexcept : seed_(mix(seed) | 1) {}
    template <class T>
    std::uint64_t operator()(const T& value) const noexcept(
        std::is_integral_v<T> || std::is_enum_v<T> || integral_pair<T>) {
        using U = std::remove_cvref_t<T>;
        if constexpr (std::is_integral_v<U> || std::is_enum_v<U>) {
            return (raw(value) ^ seed_) * UINT64_C(11995408973635179863);
        } else if constexpr (integral_pair<U>) {
            return (raw(value.first) ^ seed_) * UINT64_C(11995408973635179863) +
                   (raw(value.second) ^ seed_) * UINT64_C(10150724397891781847);
        } else return raw(value) * seed_;
    }
};
#endif

#ifndef REINSIRK_HASH_MAP_DEFINED
#define REINSIRK_HASH_MAP_DEFINED
// Flat table。検索・追加・削除は期待償却O(1)、列キーのハッシュ計算は列長に比例する。
// 再配置・clear・swap・ムーブでiterator/参照は無効。eraseは対象だけを無効にする。
// Key/Valはコピー可能、またはvalue_typeのムーブ構築がnoexceptであること。
// Hasher/KeyEqualは同値なキーに同じハッシュ値を返し、格納中に規則を変えないこと。
// 再配置を伴う新規追加の引数には、本表の要素への参照を渡さず先にコピーする。
template <class Key, class Val, class Hasher = GenericHash, class KeyEqual = std::equal_to<Key>>
class HashMap {
    // HashMap(): 空の表を構築する。必要になるまで配列は確保しない。
    // HashMap(n, hash, equal): n要素分を予約し、指定したハッシュ・同値判定で構築する。
    // operator[](key): 未登録ならVal{}で追加し、値の参照を返す。
    // find(key): 対応するiteratorを返す。未登録はend()。const版は変更できない。
    // contain(key)/contains(key): 登録済みならtrueを返す。追加はしない。
    // count(key): 登録済みなら1、未登録なら0を返す。
    // at(key): 値の参照を返す。未登録はout_of_range。const版は変更できない。
    // insert({key, value})/emplace(key, value): 未登録なら追加し、対応するiteratorを返す。
    // try_emplace(key, args...): 未登録時だけ値を構築し、{iterator, 追加の有無}を返す。
    // insert_or_assign(key, value): 追加または上書きし、{iterator, 追加の有無}を返す。
    // get_or_default(key, fallback): 登録値またはfallbackのコピーを返す。追加はしない。
    // erase(it, next=true): 対象を削除し次のiteratorを返す。false指定ならend()。
    // erase(key): キーを削除し、次のiteratorを返す。未登録・erase(end())はend()。
    // begin()/end()/cbegin()/cend(): 格納要素を順序不定で列挙する。キーは変更できない。
    // size()/empty(): 登録数・空かどうかをO(1)で返す。
    // clear(): 全要素を破棄し、確保済みの容量を再利用する。
    // reserve(n): n要素分の容量を予約する。縮小しない。
    // rehash(n): nバケット以上・現登録数に必要な容量へ再配置し、墓石を除く。
    // capacity()/load_factor(): バケット数・使用率をO(1)で返す。
    // swap(other): 内容とハッシュ・同値判定を交換する。コピーは独立した複製。
public:
    using key_type = Key;
    using mapped_type = Val;
    using value_type = std::pair<const Key, Val>;
    using Data = value_type;
    using size_type = std::size_t;
private:
    // 単純な型では全スロットを有効な値オブジェクトにし、optionalの状態領域を省く。
    // 構築に失敗しうる型はoptionalで未構築状態を管理する。
    struct CompactSlot {
        value_type value{};
        value_type& operator*() noexcept { return value; }
        const value_type& operator*() const noexcept { return value; }
        value_type* operator->() noexcept { return std::addressof(value); }
        const value_type* operator->() const noexcept { return std::addressof(value); }
        template <class... Args> void emplace(Args&&... args) {
            value_type next(std::forward<Args>(args)...);
            std::destroy_at(std::addressof(value));
            std::construct_at(std::addressof(value), std::move(next));
        }
        void reset() noexcept {} // 自明な破棄なので、削除状態だけをstate_で変更する。
    };
    using Slot = std::conditional_t<
        std::is_nothrow_default_constructible_v<value_type> &&
        std::is_nothrow_move_constructible_v<value_type> &&
        std::is_trivially_copy_constructible_v<value_type> &&
        std::is_trivially_destructible_v<value_type>, CompactSlot, std::optional<value_type>>;
    static_assert(std::is_copy_constructible_v<value_type> ||
                  std::is_nothrow_move_constructible_v<value_type>,
                  "HashMap: entries must be copyable or nothrow movable");
    static_assert(std::is_copy_constructible_v<Hasher> && std::is_copy_constructible_v<KeyEqual> &&
                  std::is_nothrow_swappable_v<Hasher> && std::is_nothrow_swappable_v<KeyEqual>,
                  "HashMap: Hasher and KeyEqual must be copyable and nothrow swappable");
    std::vector<Slot> slots_;
    std::vector<std::uint8_t> state_; // 0=空、1=登録済み、2=削除済み
    size_type size_ = 0, occupied_ = 0;
    unsigned shift_ = 64;
    [[no_unique_address]] Hasher hash_;
    [[no_unique_address]] KeyEqual equal_;
#ifndef SKIP_RUNTIME_CHECKS
    size_type generation_ = 0;
#endif
    void invalidate() noexcept {
#ifndef SKIP_RUNTIME_CHECKS
        ++generation_;
#endif
    }
    size_type max_capacity() const noexcept {
        return std::bit_floor(std::min(slots_.max_size(), state_.max_size()));
    }
    static size_type threshold(size_type capacity) noexcept { return capacity == 0 ? 0 : (capacity - 1) / 2; }
    size_type bucket(const Key& key, unsigned shift) const noexcept(
        std::is_nothrow_invocable_v<const Hasher&, const Key&>) {
        const auto h = static_cast<std::uint64_t>(hash_(key));
        if constexpr (std::is_same_v<Hasher, GenericHash>) return static_cast<size_type>(h >> shift);
        else return static_cast<size_type>((h * UINT64_C(11995408973635179863)) >> shift);
    }
    size_type locate(const Key& key) const {
        if (slots_.empty()) return 0;
        size_type p = bucket(key, shift_);
        while (state_[p] == 1) {
            if (equal_(slots_[p]->first, key)) return p;
            p = (p + 1) & (slots_.size() - 1);
        }
        if (state_[p] == 0) return slots_.size();
        while (state_[p]) {
            if constexpr (std::is_same_v<Slot, CompactSlot>) {
                if (equal_(slots_[p]->first, key)) return state_[p] == 1 ? p : slots_.size();
            } else if (state_[p] == 1 && equal_(slots_[p]->first, key)) return p;
            p = (p + 1) & (slots_.size() - 1);
        }
        return slots_.size();
    }
    void rebuild(size_type capacity) {
        if (capacity > max_capacity()) throw std::length_error("HashMap: capacity overflow");
        std::vector<Slot> next(capacity);
        std::vector<std::uint8_t> state(capacity, 0);
        const auto shift = static_cast<unsigned>(64 - std::countr_zero(capacity));
        if constexpr (std::is_nothrow_move_constructible_v<value_type> &&
                      std::is_nothrow_invocable_v<const Hasher&, const Key&>) {
            for (size_type i = 0; i < slots_.size(); ++i) {
                if (state_[i] != 1) continue;
                size_type p = bucket(slots_[i]->first, shift);
                while (state[p]) p = (p + 1) & (capacity - 1);
                next[p].emplace(std::move(*slots_[i]));
                state[p] = 1;
            }
        } else {
            // 例外を送出しうるハッシュは、移動を始める前に全移動先を計算する。
            std::vector<std::pair<size_type, size_type>> positions;
            positions.reserve(size_);
            for (size_type i = 0; i < slots_.size(); ++i) {
                if (state_[i] != 1) continue;
                size_type p = bucket(slots_[i]->first, shift);
                while (state[p]) p = (p + 1) & (capacity - 1);
                state[p] = 1;
                positions.emplace_back(i, p);
            }
            for (const auto& [old, dest] : positions) {
                if constexpr (std::is_nothrow_move_constructible_v<value_type>)
                    next[dest].emplace(std::move(*slots_[old]));
                else next[dest].emplace(*slots_[old]);
            }
        }
        slots_.swap(next);
        state_.swap(state);
        occupied_ = size_;
        shift_ = shift;
        invalidate();
    }
    size_type insertion_slot(const Key& key) {
        if (slots_.empty()) rebuild(8);
        size_type p = bucket(key, shift_), first_deleted = slots_.size();
        while (state_[p]) {
            if constexpr (std::is_same_v<Slot, CompactSlot>) {
                // 同じキーの墓石より後ろに同キーの有効要素はない。
                if (equal_(slots_[p]->first, key)) return p;
            } else if (state_[p] == 1 && equal_(slots_[p]->first, key)) return p;
            if (state_[p] == 2 && first_deleted == slots_.size()) first_deleted = p;
            p = (p + 1) & (slots_.size() - 1);
        }
        if (size_ == threshold(slots_.size())) {
            if (slots_.size() > max_capacity() / 2) throw std::length_error("HashMap: capacity overflow");
            rebuild(slots_.size() * 2);
            return insertion_slot(key);
        }
        if (first_deleted != slots_.size()) return first_deleted;
        if (occupied_ == threshold(slots_.size())) {
            rebuild(slots_.size());
            return insertion_slot(key);
        }
        return p;
    }
    void mark_inserted(size_type p) noexcept {
        if (state_[p] == 0) ++occupied_;
        state_[p] = 1;
        ++size_;
    }
public:
    template <bool Const>
    class BasicIterator {
        friend class HashMap;
        template <bool> friend class BasicIterator;
        using Owner = std::conditional_t<Const, const HashMap, HashMap>;
        Owner* owner_ = nullptr;
        size_type index_ = 0;
#ifndef SKIP_RUNTIME_CHECKS
        size_type generation_ = 0;
#endif
        BasicIterator(Owner* owner, size_type index) noexcept : owner_(owner), index_(index) {
            // 内部で新しく作るiteratorは有効な要素またはendだけを指す。
            if (index > owner->slots_.size() ||
                (index < owner->slots_.size() && owner->state_[index] != 1)) std::unreachable();
#ifndef SKIP_RUNTIME_CHECKS
            generation_ = owner->generation_;
#endif
        }
        void check(bool element) const {
#ifndef SKIP_RUNTIME_CHECKS
            if (!owner_ || generation_ != owner_->generation_ || index_ > owner_->slots_.size() ||
                (element && index_ == owner_->slots_.size()) ||
                (index_ < owner_->slots_.size() && owner_->state_[index_] != 1))
                throw std::out_of_range("HashMap: invalid iterator");
#else
            (void)element;
#endif
        }
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using iterator_concept = std::bidirectional_iterator_tag;
        using value_type = typename HashMap::value_type;
        using difference_type = std::ptrdiff_t;
        using reference = std::conditional_t<Const, const value_type&, value_type&>;
        using pointer = std::conditional_t<Const, const value_type*, value_type*>;
        BasicIterator() noexcept = default;
        template <bool Other> requires (Const && !Other)
        BasicIterator(const BasicIterator<Other>& other) noexcept : owner_(other.owner_), index_(other.index_) {
#ifndef SKIP_RUNTIME_CHECKS
            generation_ = other.generation_;
#endif
        }
        reference operator*() const { check(true); return *owner_->slots_[index_]; }
        pointer operator->() const { return std::addressof(operator*()); }
        BasicIterator& operator++() {
            check(true);
            do { ++index_; } while (index_ < owner_->slots_.size() && owner_->state_[index_] != 1);
            return *this;
        }
        BasicIterator operator++(int) { auto copy = *this; ++*this; return copy; }
        BasicIterator& operator--() {
            check(false);
            auto p = index_;
            while (p != 0) { --p; if (owner_->state_[p] == 1) { index_ = p; return *this; } }
#ifndef SKIP_RUNTIME_CHECKS
            throw std::out_of_range("HashMap: iterator before begin");
#else
            index_ = 0;
            return *this;
#endif
        }
        BasicIterator operator--(int) { auto copy = *this; --*this; return copy; }
        template <bool Other> bool operator==(const BasicIterator<Other>& other) const noexcept {
            return owner_ == other.owner_ && index_ == other.index_;
        }
    };
    using iterator = BasicIterator<false>;
    using const_iterator = BasicIterator<true>;
    using itr = iterator;

    HashMap() = default;
    explicit HashMap(size_type expected, Hasher hash = {}, KeyEqual equal = {})
        : hash_(std::move(hash)), equal_(std::move(equal)) { reserve(expected); }
    HashMap(const HashMap&) = default;
    HashMap(HashMap&& other) noexcept(std::is_nothrow_copy_constructible_v<Hasher> &&
                                    std::is_nothrow_copy_constructible_v<KeyEqual> &&
                                    std::is_nothrow_swappable_v<Hasher> &&
                                    std::is_nothrow_swappable_v<KeyEqual>)
        : hash_(other.hash_), equal_(other.equal_) { swap(other); }
    HashMap& operator=(HashMap other) { swap(other); return *this; }
    void swap(HashMap& other) noexcept(std::is_nothrow_swappable_v<Hasher> && std::is_nothrow_swappable_v<KeyEqual>) {
        // 関数オブジェクトのswapが例外を送出しないことを要求する。
        static_assert(std::is_nothrow_swappable_v<Hasher> && std::is_nothrow_swappable_v<KeyEqual>,
                      "HashMap: Hasher and KeyEqual must be nothrow swappable");
        using std::swap;
        swap(hash_, other.hash_); swap(equal_, other.equal_);
        slots_.swap(other.slots_); state_.swap(other.state_);
        swap(size_, other.size_); swap(occupied_, other.occupied_); swap(shift_, other.shift_);
        invalidate(); other.invalidate();
    }
    iterator begin() noexcept {
        size_type p = 0; while (p < slots_.size() && state_[p] != 1) ++p;
        return iterator(this, p);
    }
    const_iterator begin() const noexcept {
        size_type p = 0; while (p < slots_.size() && state_[p] != 1) ++p;
        return const_iterator(this, p);
    }
    iterator end() noexcept { return iterator(this, slots_.size()); }
    const_iterator end() const noexcept { return const_iterator(this, slots_.size()); }
    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend() const noexcept { return end(); }
    iterator find(const Key& key) { return iterator(this, locate(key)); }
    const_iterator find(const Key& key) const { return const_iterator(this, locate(key)); }
    bool contains(const Key& key) const { return locate(key) != slots_.size(); }
    bool contain(const Key& key) const { return contains(key); }
    int count(const Key& key) const { return contains(key) ? 1 : 0; }
    size_type size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    size_type capacity() const noexcept { return slots_.size(); }
    double load_factor() const noexcept { return slots_.empty() ? 0.0 : static_cast<double>(size_) / slots_.size(); }
    Val& at(const Key& key) {
        const auto p = locate(key);
        if (p == slots_.size()) throw std::out_of_range("HashMap::at: missing key");
        return slots_[p]->second;
    }
    const Val& at(const Key& key) const {
        const auto p = locate(key);
        if (p == slots_.size()) throw std::out_of_range("HashMap::at: missing key");
        return slots_[p]->second;
    }
    Val get_or_default(const Key& key, Val fallback = Val{}) const {
        const auto p = locate(key);
        return p == slots_.size() ? std::move(fallback) : slots_[p]->second;
    }
    template <class... Args>
    std::pair<iterator, bool> try_emplace(const Key& key, Args&&... args) {
        const auto p = insertion_slot(key);
        if (state_[p] == 1) return {iterator(this, p), false};
        slots_[p].emplace(std::piecewise_construct, std::forward_as_tuple(key),
                          std::forward_as_tuple(std::forward<Args>(args)...));
        mark_inserted(p);
        return {iterator(this, p), true};
    }
    Val& operator[](const Key& key) { return try_emplace(key).first->second; }
    iterator insert(const value_type& entry) { return try_emplace(entry.first, entry.second).first; }
    iterator insert(value_type&& entry) { return try_emplace(entry.first, std::move(entry.second)).first; }
    iterator emplace(const Key& key, const Val& value) { return try_emplace(key, value).first; }
    template <class V>
    std::pair<iterator, bool> insert_or_assign(const Key& key, V&& value) {
        auto result = try_emplace(key, std::forward<V>(value));
        if (!result.second) result.first->second = std::forward<V>(value);
        return result;
    }
    iterator erase(const_iterator it, bool get_next = true) {
#ifndef SKIP_RUNTIME_CHECKS
        if (it.owner_ != this || it.generation_ != generation_ || it.index_ > slots_.size())
            throw std::out_of_range("HashMap::erase: foreign or stale iterator");
#endif
        if (it.index_ == slots_.size()) return end();
#ifndef SKIP_RUNTIME_CHECKS
        if (state_[it.index_] != 1)
            throw std::out_of_range("HashMap::erase: invalid iterator");
#endif
        const auto p = it.index_;
        slots_[p].reset(); state_[p] = 2; --size_;
        if (!get_next) return end();
        auto next = p;
        do { ++next; } while (next < slots_.size() && state_[next] != 1);
        return iterator(this, next);
    }
    iterator erase(iterator it, bool get_next = true) { return erase(const_iterator(it), get_next); }
    iterator erase(const Key& key) { return erase(find(key)); }
    void clear() noexcept {
        for (size_type i = 0; i < slots_.size(); ++i) if (state_[i] == 1) slots_[i].reset();
        std::fill(state_.begin(), state_.end(), std::uint8_t{0});
        size_ = occupied_ = 0; invalidate();
    }
    void reserve(size_type count) {
        if (count <= threshold(slots_.size())) return;
        if (count > threshold(max_capacity())) throw std::length_error("HashMap::reserve: capacity overflow");
        size_type capacity = 8;
        while (count > threshold(capacity)) capacity *= 2;
        rebuild(capacity);
    }
    void rehash(size_type requested) {
        if (requested > max_capacity()) throw std::length_error("HashMap::rehash: capacity overflow");
        size_type capacity = 8;
        while (requested > capacity || size_ > threshold(capacity)) {
            if (capacity > max_capacity() / 2) throw std::length_error("HashMap::rehash: capacity overflow");
            capacity *= 2;
        }
        rebuild(capacity);
    }
};
#endif
