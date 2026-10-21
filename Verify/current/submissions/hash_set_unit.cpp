#define SKIP_RUNTIME_CHECKS
// Verify: https://judge.yosupo.jp/problem/aplusb

// Library: DataStructure/Hash_set.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/hashmap/hashset.hpp
#include <algorithm>
#include <bit>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
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

#ifndef REINSIRK_HASH_SET_DEFINED
#define REINSIRK_HASH_SET_DEFINED
// Flat table。検索・追加・次iteratorを求めない削除は期待償却O(1)。列キーのハッシュ計算は列長に比例する。
// 次iteratorへの移動は空き区間長に比例する。削除では自動縮小しないため、疎になればrehash(0)で縮小する。
// 再配置・clear・swap・ムーブでiterator/参照は無効。eraseは対象だけを無効にする。
// Keyはコピー可能、またはムーブ構築がnoexceptであること。
// Hasher/KeyEqualは同値なキーに同じハッシュ値を返し、格納中に規則を変えないこと。
// iterator/参照の無効化後の使用は禁止。検査でも全ての使い回しは検出しない。
template <class Key, class Hasher = GenericHash, class KeyEqual = std::equal_to<Key>>
class HashSet {
    // HashSet()/HashSet(n, hash, equal): 空集合を作り、後者はn要素分を予約する。
    // HashSet({keys}, hash, equal)/HashSet(first,last,hash,equal): 指定したキー列から構築する。
    // insert(const Key&)/insert(Key&&): コピー/ムーブで追加しiteratorを返す。重複時は既存要素を返す。
    // insert_checked(const Key&)/insert_checked(Key&&): コピー/ムーブで追加し、{iterator, 追加の有無}を返す。
    // emplace(args...): Keyを構築して追加し、{iterator, 追加の有無}を返す。
    // insert_range(first,last): 入力iteratorの範囲を追加する。同じ表のiteratorは渡さないこと。
    // find(key): 対応するiteratorを返す。未登録はend()。キーの変更は禁止する。
    // contain(key)/contains(key)/count(key): 登録済みか、または登録数0/1を返す。
    // erase(it, next=true): 削除して次のiteratorを返す。falseならO(1)でend()。erase(end())もend()。
    // erase(key, next=true): キーを削除して次のiteratorを返す。falseなら期待O(1)でend()。未登録もend()。
    // erase_if(pred): pred(const Key&)がtrueの要素を削除し、削除数を返す。O(capacity())。predは表を変更しないこと。
    // begin()/end()/cbegin()/cend(): 順序不定で列挙する。逆向き移動もでき、キーはconst参照。
    // size()/empty()/capacity()/load_factor(): 要素数・空か・バケット数・使用率をO(1)で返す。
    // clear(): O(capacity())で全要素を破棄し、確保済み容量を再利用する。
    // reserve(n): n要素分を予約する。縮小しない。再配置はO(旧容量+新容量)。
    // rehash(n): nバケット以上・現登録数に必要な容量へ再配置し、墓石を除く。
    // HashSet(other)/operator=(other): 独立したコピーを作る。コピーにはKeyのコピー構築が必要。
    // HashSet(move(other))/operator=(move(other)): 内容を移動し、移動元は空で再利用可能。
    // swap(other): 内容とハッシュ・同値判定を交換する。O(1)。
    // operator==(other)/!=: 要素数と両方向のcontainsで比較する。期待O(両表の容量)。
public:
    using key_type = Key;
    using value_type = Key;
    using Data = value_type;
    using size_type = std::size_t;
private:
    // 自明にコピーできる型はunionに格納し、空きスロットの構築とoptionalの状態領域を省く。
    // 構築に失敗しうる型はoptionalで未構築状態を管理する。
    union CompactSlot {
        std::remove_cv_t<value_type> value;
        // 新しいスロットでは値を構築しない。unionの暗黙copy/moveはオブジェクト表現をコピーする。
        CompactSlot() noexcept {}
        value_type& operator*() noexcept { return value; }
        const value_type& operator*() const noexcept { return value; }
        value_type* operator->() noexcept { return std::addressof(value); }
        const value_type* operator->() const noexcept { return std::addressof(value); }
        template <class... Args> void emplace(Args&&... args) {
            value_type next(std::forward<Args>(args)...);
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
                  "HashSet: entries must be copyable or nothrow movable");
    static_assert(std::is_copy_constructible_v<Hasher> && std::is_copy_constructible_v<KeyEqual> &&
                  std::is_nothrow_swappable_v<Hasher> && std::is_nothrow_swappable_v<KeyEqual>,
                  "HashSet: Hasher and KeyEqual must be copyable and nothrow swappable");
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
        while (state_[p]) {
            if (state_[p] == 1 && equal_(*slots_[p], key)) return p;
            p = (p + 1) & (slots_.size() - 1);
        }
        return slots_.size();
    }
    void rebuild(size_type capacity) {
        if (capacity > max_capacity()) throw std::length_error("HashSet: capacity overflow");
        std::vector<Slot> next(capacity);
        std::vector<std::uint8_t> state(capacity, 0);
        const auto shift = static_cast<unsigned>(64 - std::countr_zero(capacity));
        if constexpr (std::is_nothrow_move_constructible_v<value_type> &&
                      std::is_nothrow_invocable_v<const Hasher&, const Key&>) {
            for (size_type i = 0; i < slots_.size(); ++i) {
                if (state_[i] != 1) continue;
                size_type p = bucket(*slots_[i], shift);
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
                size_type p = bucket(*slots_[i], shift);
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
        for (;;) {
            size_type p = bucket(key, shift_);
            // 削除前は墓石の分岐と探索位置の記録を省く。
            if (occupied_ == size_) {
                while (state_[p]) {
                    if (equal_(*slots_[p], key)) return p;
                    p = (p + 1) & (slots_.size() - 1);
                }
                if (size_ != threshold(slots_.size())) return p;
                if (slots_.size() > max_capacity() / 2) throw std::length_error("HashSet: capacity overflow");
                rebuild(slots_.size() * 2);
                continue;
            }
            size_type first_deleted = slots_.size();
            while (state_[p]) {
                if (state_[p] == 1 && equal_(*slots_[p], key)) return p;
                if (state_[p] == 2 && first_deleted == slots_.size()) first_deleted = p;
                p = (p + 1) & (slots_.size() - 1);
            }
            if (first_deleted != slots_.size()) return first_deleted;
            if (occupied_ != threshold(slots_.size())) return p;
            if (size_ == occupied_) {
                if (slots_.size() > max_capacity() / 2) throw std::length_error("HashSet: capacity overflow");
                rebuild(slots_.size() * 2);
            } else rebuild(slots_.size());
        }
    }
    void mark_inserted(size_type p) noexcept {
        if (state_[p] == 0) ++occupied_;
        state_[p] = 1;
        ++size_;
    }
public:
    class Iterator {
        friend class HashSet;
        const HashSet* owner_ = nullptr;
        size_type index_ = 0;
#ifndef SKIP_RUNTIME_CHECKS
        size_type generation_ = 0;
#endif
        Iterator(const HashSet* owner, size_type index) noexcept : owner_(owner), index_(index) {
#ifndef SKIP_RUNTIME_CHECKS
            generation_ = owner->generation_;
#endif
        }
        void check(bool element) const {
#ifndef SKIP_RUNTIME_CHECKS
            if (!owner_ || generation_ != owner_->generation_ || index_ > owner_->slots_.size() ||
                (element && index_ == owner_->slots_.size()) ||
                (index_ < owner_->slots_.size() && owner_->state_[index_] != 1))
                throw std::out_of_range("HashSet: invalid iterator");
#else
            (void)element;
#endif
        }
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using iterator_concept = std::bidirectional_iterator_tag;
        using value_type = Key;
        using difference_type = std::ptrdiff_t;
        using reference = const Key&;
        using pointer = const Key*;
        Iterator() noexcept = default;
        reference operator*() const { check(true); return *owner_->slots_[index_]; }
        pointer operator->() const { return std::addressof(operator*()); }
        Iterator& operator++() {
            check(true);
            do { ++index_; } while (index_ < owner_->slots_.size() && owner_->state_[index_] != 1);
            return *this;
        }
        Iterator operator++(int) { auto copy = *this; ++*this; return copy; }
        Iterator& operator--() {
            check(false);
            auto p = index_;
            while (p != 0) { --p; if (owner_->state_[p] == 1) { index_ = p; return *this; } }
#ifndef SKIP_RUNTIME_CHECKS
            throw std::out_of_range("HashSet: iterator before begin");
#else
            index_ = 0;
            return *this;
#endif
        }
        Iterator operator--(int) { auto copy = *this; --*this; return copy; }
        bool operator==(const Iterator& other) const noexcept {
            return owner_ == other.owner_ && index_ == other.index_;
        }
    };
    using iterator = Iterator;
    using const_iterator = Iterator;
    using itr = iterator;

    HashSet() = default;
    explicit HashSet(size_type expected, Hasher hash = {}, KeyEqual equal = {})
        : hash_(std::move(hash)), equal_(std::move(equal)) { reserve(expected); }
    HashSet(std::initializer_list<Key> keys, Hasher hash = {}, KeyEqual equal = {})
        : HashSet(keys.size(), std::move(hash), std::move(equal)) { insert_range(keys.begin(), keys.end()); }
    template <std::input_iterator It, std::sentinel_for<It> Sentinel>
    HashSet(It first, Sentinel last, Hasher hash = {}, KeyEqual equal = {})
        : hash_(std::move(hash)), equal_(std::move(equal)) { insert_range(first, last); }
    HashSet(const HashSet&) = default;
    HashSet(HashSet&& other) noexcept(std::is_nothrow_copy_constructible_v<Hasher> &&
                                    std::is_nothrow_copy_constructible_v<KeyEqual> &&
                                    std::is_nothrow_swappable_v<Hasher> &&
                                    std::is_nothrow_swappable_v<KeyEqual>)
        : hash_(other.hash_), equal_(other.equal_) { swap(other); }
    HashSet& operator=(HashSet other) { swap(other); return *this; }
    void swap(HashSet& other) noexcept(std::is_nothrow_swappable_v<Hasher> && std::is_nothrow_swappable_v<KeyEqual>) {
        // 関数オブジェクトのswapが例外を送出しないことを要求する。
        static_assert(std::is_nothrow_swappable_v<Hasher> && std::is_nothrow_swappable_v<KeyEqual>,
                      "HashSet: Hasher and KeyEqual must be nothrow swappable");
        using std::swap;
        swap(hash_, other.hash_); swap(equal_, other.equal_);
        slots_.swap(other.slots_); state_.swap(other.state_);
        swap(size_, other.size_); swap(occupied_, other.occupied_); swap(shift_, other.shift_);
        invalidate(); other.invalidate();
    }
    iterator begin() const noexcept {
        size_type p = 0; while (p < slots_.size() && state_[p] != 1) ++p;
        return iterator(this, p);
    }
    iterator end() const noexcept { return iterator(this, slots_.size()); }
    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend() const noexcept { return end(); }
    friend iterator begin(const HashSet& set) noexcept { return set.begin(); }
    friend iterator end(const HashSet& set) noexcept { return set.end(); }
    iterator find(const Key& key) const { return iterator(this, locate(key)); }
    bool contains(const Key& key) const { return locate(key) != slots_.size(); }
    bool contain(const Key& key) const { return contains(key); }
    int count(const Key& key) const { return contains(key) ? 1 : 0; }
    size_type size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }
    size_type capacity() const noexcept { return slots_.size(); }
    double load_factor() const noexcept { return slots_.empty() ? 0.0 : static_cast<double>(size_) / slots_.size(); }
private:
    template <class K>
    std::pair<iterator, bool> insert_checked_impl(K&& key) {
        const auto p = insertion_slot(key);
        if (state_[p] == 1) return {iterator(this, p), false};
        slots_[p].emplace(std::forward<K>(key));
        mark_inserted(p);
        return {iterator(this, p), true};
    }
public:
    std::pair<iterator, bool> insert_checked(const Key& key) { return insert_checked_impl(key); }
    std::pair<iterator, bool> insert_checked(Key&& key) { return insert_checked_impl(std::move(key)); }
    iterator insert(const Key& key) { return insert_checked(key).first; }
    iterator insert(Key&& key) { return insert_checked(std::move(key)).first; }
    template <class... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        Key key(std::forward<Args>(args)...);
        return insert_checked(std::move(key));
    }
    template <std::input_iterator It, std::sentinel_for<It> Sentinel>
    void insert_range(It first, Sentinel last) {
        for (; first != last; ++first) insert(*first);
    }
    iterator erase(const_iterator it, bool get_next = true) {
#ifndef SKIP_RUNTIME_CHECKS
        if (it.owner_ != this || it.generation_ != generation_) throw std::out_of_range("HashSet::erase: foreign or stale iterator");
#endif
        if (it.index_ == slots_.size()) return end();
        it.check(true);
        const auto p = it.index_;
        slots_[p].reset(); state_[p] = 2; --size_;
        if (!get_next) return end();
        auto next = iterator(this, p);
        do { ++next.index_; } while (next.index_ < slots_.size() && state_[next.index_] != 1);
        return next;
    }
    iterator erase(const Key& key, bool get_next = true) { return erase(find(key), get_next); }
    template <class Predicate>
    size_type erase_if(Predicate predicate) {
        size_type removed = 0;
        for (size_type i = 0; i < slots_.size(); ++i) {
            if (state_[i] == 1 && std::invoke(predicate, std::as_const(*slots_[i]))) {
                slots_[i].reset(); state_[i] = 2; --size_; ++removed;
            }
        }
        return removed;
    }
    friend void swap(HashSet& left, HashSet& right) noexcept { left.swap(right); }
    friend bool operator==(const HashSet& left, const HashSet& right) {
        if (left.size_ != right.size_) return false;
        for (const auto& key : left) if (!right.contains(key)) return false;
        for (const auto& key : right) if (!left.contains(key)) return false;
        return true;
    }
    void clear() noexcept {
        for (size_type i = 0; i < slots_.size(); ++i) if (state_[i] == 1) slots_[i].reset();
        std::fill(state_.begin(), state_.end(), std::uint8_t{0});
        size_ = occupied_ = 0; invalidate();
    }
    void reserve(size_type count) {
        if (count <= threshold(slots_.size())) return;
        if (count > threshold(max_capacity())) throw std::length_error("HashSet::reserve: capacity overflow");
        size_type capacity = 8;
        while (count > threshold(capacity)) capacity *= 2;
        rebuild(capacity);
    }
    void rehash(size_type requested) {
        if (requested > max_capacity()) throw std::length_error("HashSet::rehash: capacity overflow");
        size_type capacity = 8;
        while (requested > capacity || size_ > threshold(capacity)) {
            if (capacity > max_capacity() / 2) throw std::length_error("HashSet::rehash: capacity overflow");
            capacity *= 2;
        }
        rebuild(capacity);
    }
};
#endif


// SPDX-License-Identifier: CC0-1.0
// Verify I/O: Verify/current/common/fast_io.hpp
// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef REINSIRK_VERIFY_FAST_IO_DEFINED
#define REINSIRK_VERIFY_FAST_IO_DEFINED

// VerifyInput(file=stdin): 非所有のFILEから読む。別の入力方式と混用しない。
// >> 整数: 64bit以下の十進整数を読む。検査省略時は値域内の正しい入力を前提とする。
// >> char/string: 空白を飛ばし、1文字／空白までの文字列を読む。
// >> 浮動小数点: 十進文字列を変換する。通常のCロケールで使う。
// bool / !: 直前までの読み取りが成功しているか／失敗したかを返す。
class VerifyInput {
    static constexpr std::size_t buffer_size = 1 << 17;
    std::FILE* file_;
    std::array<unsigned char, buffer_size> buffer_;
    std::size_t position_ = 0, size_ = 0;
    bool good_ = true;

    int peek() {
        if (position_ == size_) {
            size_ = std::fread(buffer_.data(), 1, buffer_.size(), file_);
            position_ = 0;
            if (size_ == 0) return EOF;
        }
        return buffer_[position_];
    }

    int skip_space() {
        int c;
        while ((c = peek()) != EOF && c <= ' ') ++position_;
        if (c == EOF) good_ = false;
        return c;
    }

public:
    explicit VerifyInput(std::FILE* file = stdin) : file_(file) {}
    VerifyInput(const VerifyInput&) = delete;
    VerifyInput& operator=(const VerifyInput&) = delete;

    explicit operator bool() const noexcept { return good_; }
    bool operator!() const noexcept { return !good_; }

    template<class T>
        requires (std::is_integral_v<T> && sizeof(T) <= sizeof(std::uint64_t)
                  && !std::is_same_v<T, char>)
    VerifyInput& operator>>(T& result) {
        if (!good_) return *this;
        int c = skip_space();
        if (c == EOF) return *this;
        bool negative = false;
        if (c == '-' || c == '+') {
            negative = c == '-';
            ++position_;
            c = peek();
        }
        if (c == EOF) {
            good_ = false;
            return *this;
        }
#ifndef SKIP_RUNTIME_CHECKS
        if (c < '0' || c > '9') {
            good_ = false;
            return *this;
        }
        // 先頭の0を除けば、20桁以下の末尾直前の値はuint64_tに収まる。
        while (c == '0') {
            ++position_;
            c = peek();
        }
        std::uint64_t previous = 0;
        unsigned last_digit = 0;
        std::size_t digits = 0;
#endif
        std::uint64_t value = 0;
        while (c >= '0' && c <= '9') {
#ifndef SKIP_RUNTIME_CHECKS
            previous = value;
            last_digit = static_cast<unsigned>(c - '0');
            if (digits <= 20) ++digits;
#endif
            value = value * 10 + static_cast<unsigned>(c - '0');
            ++position_;
            c = peek();
        }
#ifndef SKIP_RUNTIME_CHECKS
        constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
        if ((c != EOF && c > ' ') || digits > 20
            || (digits == 20 && (previous > maximum / 10
                || (previous == maximum / 10 && last_digit > maximum % 10)))) {
            good_ = false;
            return *this;
        }
        std::uint64_t limit = static_cast<std::uint64_t>(std::numeric_limits<T>::max());
        if constexpr (std::is_signed_v<T>) limit += negative;
        else if (negative && value != 0) good_ = false;
        if (value > limit) good_ = false;
        if (!good_) return *this;
#endif
        if constexpr (std::is_signed_v<T>) {
            constexpr std::uint64_t minimum_magnitude =
                static_cast<std::uint64_t>(std::numeric_limits<T>::max()) + 1;
            // -(最小値)は評価せず、絶対値が最大値+1のときだけ最小値を使う。
            result = negative
                ? (value == minimum_magnitude ? std::numeric_limits<T>::min()
                                  : static_cast<T>(-static_cast<T>(value)))
                : static_cast<T>(value);
        } else result = static_cast<T>(value);
        return *this;
    }

    VerifyInput& operator>>(char& result) {
        if (!good_) return *this;
        const int c = skip_space();
        if (c != EOF) {
            result = static_cast<char>(c);
            ++position_;
        }
        return *this;
    }

    VerifyInput& operator>>(std::string& result) {
        if (!good_) return *this;
        int c = skip_space();
        if (c == EOF) return *this;
        result.clear();
        do {
            result.push_back(static_cast<char>(c));
            ++position_;
            c = peek();
        } while (c != EOF && c > ' ');
        return *this;
    }

    template<class T> requires std::is_floating_point_v<T>
    VerifyInput& operator>>(T& result) {
        std::string token;
        *this >> token;
        if (!good_) return *this;
#ifndef SKIP_RUNTIME_CHECKS
        char* end = nullptr;
        const long double value = std::strtold(token.c_str(), &end);
        if (end != token.c_str() + token.size()) good_ = false;
        else result = static_cast<T>(value);
#else
        result = static_cast<T>(std::strtold(token.c_str(), nullptr));
#endif
        return *this;
    }
};

// VerifyOutput(file=stdout): 非所有のFILEへ書く。破棄時にflushし、FILEは閉じない。
// << 整数/char/文字列/浮動小数点: 値を出力する。整数は64bit以下に対応する。
// fixed(digits): 浮動小数点を小数点以下digits桁で出力する（digits>=0）。
// flush(): バッファの内容をFILEへ渡す。bool / !で書き込み失敗を確認できる。
class VerifyOutput {
    static constexpr std::size_t buffer_size = 1 << 17;
    std::FILE* file_;
    std::array<char, buffer_size> buffer_;
    std::size_t size_ = 0;
    int precision_ = 6;
    bool fixed_ = false, good_ = true;

    void write(std::string_view text) {
        while (!text.empty() && good_) {
            if (size_ == buffer_.size()) flush();
            const auto count = std::min(text.size(), buffer_.size() - size_);
            std::memcpy(buffer_.data() + size_, text.data(), count);
            size_ += count;
            text.remove_prefix(count);
        }
    }

public:
    explicit VerifyOutput(std::FILE* file = stdout) : file_(file) {}
    VerifyOutput(const VerifyOutput&) = delete;
    VerifyOutput& operator=(const VerifyOutput&) = delete;
    ~VerifyOutput() { flush(); }

    explicit operator bool() const noexcept { return good_; }
    bool operator!() const noexcept { return !good_; }

    VerifyOutput& fixed(int digits) {
        if (digits < 0) throw std::invalid_argument("negative output precision");
        fixed_ = true;
        precision_ = digits;
        return *this;
    }

    void flush() noexcept {
        std::size_t written = 0;
        while (written < size_ && good_) {
            const auto count = std::fwrite(buffer_.data() + written, 1, size_ - written, file_);
            if (count == 0) good_ = false;
            written += count;
        }
        size_ = 0;
    }

    template<class T>
        requires (std::is_integral_v<T> && sizeof(T) <= sizeof(std::uint64_t)
                  && !std::is_same_v<T, char>)
    VerifyOutput& operator<<(T value) {
        if constexpr (std::is_same_v<T, bool>) return *this << static_cast<char>('0' + value);
        else {
            char digits[std::numeric_limits<std::uint64_t>::digits10 + 3];
            const auto end = std::to_chars(digits, digits + sizeof(digits), value).ptr;
            write(std::string_view(digits, static_cast<std::size_t>(end - digits)));
            return *this;
        }
    }

    VerifyOutput& operator<<(char value) {
        if (size_ == buffer_.size()) flush();
        if (good_) buffer_[size_++] = value;
        return *this;
    }

    VerifyOutput& operator<<(std::string_view value) { write(value); return *this; }
    VerifyOutput& operator<<(const std::string& value) { write(value); return *this; }
    VerifyOutput& operator<<(const char* value) { write(value); return *this; }

    template<class T> requires std::is_floating_point_v<T>
    VerifyOutput& operator<<(T value) {
        char local[128];
        const char* format = fixed_ ? "%.*Lf" : "%.*Lg";
        const int count = std::snprintf(local, sizeof(local), format, precision_,
                                        static_cast<long double>(value));
        if (count < 0) good_ = false;
        else if (static_cast<std::size_t>(count) < sizeof(local))
            write(std::string_view(local, static_cast<std::size_t>(count)));
        else {
            std::vector<char> text(static_cast<std::size_t>(count) + 1);
            std::snprintf(text.data(), text.size(), format, precision_, static_cast<long double>(value));
            write(std::string_view(text.data(), static_cast<std::size_t>(count)));
        }
        return *this;
    }
};

inline VerifyInput verify_in;
inline VerifyOutput verify_out;
#endif

#include <algorithm>
#include <cstdint>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

int main() {
    auto require = [](bool value) { if (!value) throw std::runtime_error("HashSet verification failed"); };
    std::mt19937_64 random(20261019);
    HashSet<int> values;
    std::set<int> reference;
    values.reserve(256);
    for (int step = 0; step < 5000; ++step) {
        int key = static_cast<int>(random() % 257) - 128;
        if (random() & 1) {
            auto [it, added] = values.insert_checked(key);
            auto [expected_it, expected_added] = reference.insert(key);
            require(added == expected_added && *it == *expected_it);
        } else {
            values.erase(key, false); reference.erase(key);
        }
        require(values.size() == reference.size() && values.empty() == reference.empty());
        require(values.contains(key) == reference.contains(key));
        require(values.count(key) == static_cast<int>(reference.count(key)));
        std::set<int> enumerated(values.begin(), values.end()); require(enumerated == reference);
        const auto& const_values = values;
        require((const_values.find(key) != const_values.end()) == reference.contains(key));
    }
    auto copy = values;
    values.clear(); require(values.empty());
    std::set<int> enumerated(copy.begin(), copy.end()); require(enumerated == reference);
    auto moved = std::move(copy); require(copy.empty());
    require(std::set<int>(moved.begin(), moved.end()) == reference);
    auto removed = moved.erase_if([](int x) { return x < 0; });
    auto expected_removed = std::erase_if(reference, [](int x) { return x < 0; });
    require(removed == expected_removed && std::set<int>(moved.begin(), moved.end()) == reference);
    HashSet<std::pair<int, int>> pairs;
    pairs.insert({-1, 2}); pairs.insert({-1, 2}); pairs.insert({2, -1});
    require(pairs.size() == 2 && pairs.contains({-1, 2}));
    HashSet<std::string> strings;
    strings.insert(""); strings.insert("abc"); strings.insert(std::string("a\0b", 3));
    require(strings.size() == 3 && strings.contains(std::string("a\0b", 3)));
    HashSet<std::vector<int>> vectors;
    vectors.insert({1, 2}); vectors.insert({2, 1}); vectors.insert({});
    require(vectors.size() == 3 && vectors.contains({1, 2}) && vectors.contains({}));
    std::int64_t a, b; if (verify_in >> a >> b) verify_out << a + b << '\n';
}
