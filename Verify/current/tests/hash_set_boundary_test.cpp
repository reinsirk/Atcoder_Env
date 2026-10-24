// SPDX-License-Identifier: CC0-1.0
#ifndef HEADER
#define HEADER "../../../DataStructure/Hash_set.cpp"
#endif
#include HEADER
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>

// 末尾バケットに衝突させ、先頭へ回り込む探索列を作る。
struct WrapHash {
    std::uint64_t operator()(int) const noexcept {
        constexpr std::uint64_t multiplier = UINT64_C(11995408973635179863);
        std::uint64_t inverse = 1;
        for (int i = 0; i < 6; ++i) inverse *= 2 - multiplier * inverse;
        return UINT64_MAX * inverse;
    }
};
struct ResidueHash {
    std::uint64_t operator()(int x) const noexcept { return GenericHash(23)(x % 10); }
};
struct ResidueEqual {
    bool operator()(int x, int y) const noexcept { return x % 10 == y % 10; }
};
struct MoveValue {
    int value;
    MoveValue() noexcept : value(37) {}
    explicit MoveValue(int x) noexcept : value(x) {}
    MoveValue(const MoveValue&) = default;
    MoveValue(MoveValue&& other) noexcept : value(other.value) { other.value = -1; }
    bool operator==(const MoveValue&) const = default;
};
struct MoveHash {
    static inline bool armed = false;
    std::uint64_t operator()(const MoveValue& x) const {
        if (armed && x.value == 17) throw std::runtime_error("hash");
        return GenericHash(9)(x.value);
    }
};
static_assert(std::is_trivially_copy_constructible_v<MoveValue>);
static_assert(std::is_trivially_destructible_v<MoveValue>);
static_assert(std::is_nothrow_move_constructible_v<MoveValue>);
static_assert(!std::is_trivially_move_constructible_v<MoveValue>);
struct alignas(128) AlignedKey {
    std::array<std::uint64_t, 8> a{};
    bool operator==(const AlignedKey&) const = default;
};
struct AlignedHash {
    std::uint64_t operator()(const AlignedKey& key) const noexcept { return GenericHash(11)(key.a[0]); }
};
struct ThrowScalar {
    bool fail;
    operator int() const {
        if (fail) throw std::runtime_error("conversion");
        return 73;
    }
};
template<class Exception, class Action> void throws(Action action) {
    bool caught = false;
    try { action(); } catch (const Exception&) { caught = true; }
    assert(caught);
}
int main() {
    static_assert(std::bidirectional_iterator<HashSet<int>::iterator>);
    static_assert(std::is_const_v<std::remove_reference_t<decltype(*HashSet<int>{}.begin())>>);
    HashSet<int, WrapHash> wrapped(0, WrapHash{});
    std::set<int> reference;
    for (int cycle = 0; cycle < 50; ++cycle) {
        for (int key = 0; key < 80; ++key) {
            auto got = wrapped.insert_checked(key);
            auto want = reference.insert(key);
            assert(got.second == want.second && *got.first == *want.first);
        }
        for (int key = cycle % 3; key < 80; key += 3) {
            wrapped.erase(key, false);
            reference.erase(key);
        }
        auto copy = wrapped;
        assert(std::set<int>(copy.begin(), copy.end()) == reference);
        for (int key = 0; key < 80; ++key) {
            assert(copy.contains(key) == reference.contains(key));
            copy.insert(key);
        }
        assert(copy.size() == 80 && wrapped.size() == reference.size());
        std::set<int> reversed;
        for (auto it = wrapped.end(); it != wrapped.begin();) reversed.insert(*--it);
        assert(reversed == reference);
        wrapped.rehash(cycle % 2 ? 8 : 512);
        assert(std::set<int>(wrapped.begin(), wrapped.end()) == reference);
    }

    // 未構築スロット・墓石・clear後の領域を含む独立したコピー。
    HashSet<const int> sparse(65536, GenericHash(13));
    auto empty_copy = sparse;
    assert(empty_copy.empty() && empty_copy.capacity() == sparse.capacity());
    for (int key = -100; key <= 100; ++key) sparse.insert(key);
    for (int key = -100; key <= 100; key += 3) sparse.erase(key, false);
    auto copy = sparse;
    HashSet<const int> assigned;
    assigned.insert(1000);
    assigned = sparse;
    assert(copy == sparse && assigned == sparse);
    for (int key = -100; key <= 100; ++key) copy.insert(key);
    assert(copy.size() == 201 && copy != sparse);
    sparse.clear();
    auto cleared = sparse;
    cleared.insert(7);
    empty_copy.insert(9);
    assert(sparse.empty() && cleared.contains(7) && empty_copy.contains(9));
    sparse.emplace(5);
    sparse.emplace(5);
    assert(sparse.size() == 1 && sparse.contains(5));

    HashSet<int, ResidueHash, ResidueEqual> equivalent(0, ResidueHash{}, ResidueEqual{});
    assert(equivalent.insert_checked(3).second);
    assert(!equivalent.insert_checked(13).second && equivalent.contains(23));
    equivalent.erase(33, false);
    assert(equivalent.empty());
    equivalent.insert(43);
    assert(equivalent.contains(3));

    // 自明なコピーと非自明なムーブを持つキーの再配置・例外。
    HashSet<MoveValue, MoveHash> moves;
    for (int key = 0; key < 100; ++key) moves.emplace(key);
    for (int key = 0; key < 100; key += 3) moves.erase(MoveValue(key), false);
    auto move_copy = moves;
    const auto old_capacity = moves.capacity();
    MoveHash::armed = true;
    throws<std::runtime_error>([&] { moves.reserve(4096); });
    assert(moves.capacity() == old_capacity && moves.size() == move_copy.size());
    throws<std::runtime_error>([&] { moves.rehash(8192); });
    MoveHash::armed = false;
    for (int key = 0; key < 100; ++key)
        assert(moves.contains(MoveValue(key)) == (key % 3 != 0));
    move_copy.reserve(8192);
    assert(move_copy == moves);
    auto moved = std::move(move_copy);
    assert(move_copy.empty() && moved == moves);
    move_copy.emplace(500);
    assert(move_copy.contains(MoveValue(500)));

    HashSet<int> conversions(32, GenericHash(7));
    throws<std::runtime_error>([&] { conversions.emplace(ThrowScalar{true}); });
    assert(conversions.empty());
    conversions.emplace(ThrowScalar{false});
    conversions.erase(73, false);
    throws<std::runtime_error>([&] { conversions.emplace(ThrowScalar{true}); });
    assert(conversions.empty() && !conversions.contains(73));
    conversions.emplace(ThrowScalar{false});
    assert(conversions.size() == 1 && conversions.contains(73));

    HashSet<AlignedKey, AlignedHash> aligned;
    for (std::uint64_t i = 0; i < 257; ++i) { AlignedKey key; key.a[0] = i; aligned.insert(key); }
    for (const auto& key : aligned)
        assert(reinterpret_cast<std::uintptr_t>(std::addressof(key)) % alignof(AlignedKey) == 0);
    auto aligned_copy = aligned;
    aligned_copy.reserve(8192);
    assert(aligned_copy == aligned);
    HashSet<int> capacity;
    throws<std::length_error>([&] { capacity.reserve(std::numeric_limits<std::size_t>::max()); });
    throws<std::length_error>([&] { capacity.rehash(std::numeric_limits<std::size_t>::max()); });
    assert(capacity.empty());
#ifndef SKIP_RUNTIME_CHECKS
    HashSet<int> checked{1, 2}, foreign{3};
    throws<std::out_of_range>([&] { auto it = checked.end(); ++it; });
    throws<std::out_of_range>([&] { auto it = checked.begin(); --it; });
    throws<std::out_of_range>([&] { checked.erase(foreign.begin()); });
    throws<std::out_of_range>([&] { HashSet<int>::iterator it; checked.erase(it); });
    auto stale = checked.begin();
    checked.rehash(1024);
    throws<std::out_of_range>([&] { (void)*stale; });
#endif
    std::cout << "PASS wraparound/tombstone/cv-key copies, custom equality, nontrivial move/hash exception, alignment, iterator/capacity limits\n";
}
