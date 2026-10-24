// SPDX-License-Identifier: CC0-1.0
#ifndef HEADER
#define HEADER "../../../DataStructure/Hash_map.cpp"
#endif
#include HEADER
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <type_traits>
#include <utility>

struct StorageCollision {
    std::uint64_t operator()(int) const noexcept { return 0; }
};
struct MoveValue {
    int value;
    MoveValue() noexcept : value(37) {}
    explicit MoveValue(int x) noexcept : value(x) {}
    MoveValue(const MoveValue&) = default;
    MoveValue(MoveValue&& other) noexcept : value(other.value) { other.value = -1; }
};
using MovePair = std::pair<const int, MoveValue>;
static_assert(std::is_trivially_copy_constructible_v<MovePair>);
static_assert(std::is_trivially_destructible_v<MovePair>);
static_assert(std::is_nothrow_move_constructible_v<MovePair>);
static_assert(std::is_nothrow_default_constructible_v<MovePair>);
static_assert(!std::is_trivially_move_constructible_v<MovePair>);
struct alignas(64) AlignedValue {
    std::array<std::int64_t, 8> words{};
};
struct ThrowScalar {
    bool fail;
    operator std::int64_t() const {
        if (fail) throw std::runtime_error("conversion");
        return 73;
    }
};
template <class Action> void conversion_throws(Action action) {
    bool caught = false;
    try { action(); } catch (const std::runtime_error&) { caught = true; }
    assert(caught);
}
int main() {
    // 未使用領域が大半の表をコピーし、元の表と独立に変更できることを確認する。
    HashMap<int, std::int64_t> original(65536, GenericHash(123));
    auto empty_copy = original;
    assert(empty_copy.empty() && empty_copy.capacity() == original.capacity());
    std::map<int, std::int64_t> reference;
    for (int key = -128; key <= 128; ++key) {
        original[key] = std::int64_t{key} * key - 5;
        reference[key] = std::int64_t{key} * key - 5;
    }
    for (int key = -128; key <= 128; key += 3) {
        original.erase(key, false);
        reference.erase(key);
    }
    auto copy = original;
    HashMap<int, std::int64_t> assigned;
    assigned[10000] = 99;
    assigned = original;
    for (const auto& [key, value] : reference) {
        assert(copy.at(key) == value && assigned.at(key) == value);
    }
    for (int key = -128; key <= 128; key += 3) copy[key] = -key;
    assert(copy.size() == 257 && original.size() == reference.size());
    for (int key = -128; key <= 128; ++key)
        assert(copy.at(key) == (reference.contains(key) ? reference.at(key) : -key));
    original.clear();
    auto cleared_copy = original;
    cleared_copy[5] = 11;
    assert(original.empty() && cleared_copy.size() == 1 && cleared_copy.at(5) == 11);
    empty_copy[1] = 8;
    assert(empty_copy.at(1) == 8 && assigned.size() == reference.size());

    // 自明なコピーと非自明なムーブを併せ持つ値も、コピー・墓石・再配置を維持する。
    HashMap<int, MoveValue> moves(0, GenericHash(987));
    for (int key = 0; key < 257; ++key) moves.try_emplace(key, key + 5);
    assert(moves[1000].value == 37);
    for (int key = 0; key < 257; key += 3) moves.erase(key, false);
    auto move_copy = moves;
    move_copy.reserve(4096);
    for (int key = 0; key < 257; ++key) {
        if (key % 3 == 0) assert(move_copy.try_emplace(key, key + 9).second);
        else assert(move_copy.at(key).value == key + 5 && moves.at(key).value == key + 5);
    }
    for (int key = 0; key < 257; ++key)
        assert(move_copy.at(key).value == key + (key % 3 == 0 ? 9 : 5));
    auto moved = std::move(move_copy);
    assert(move_copy.empty() && moved.size() == 258);
    move_copy[1].value = 19;
    assert(move_copy.at(1).value == 19 && moved.at(1).value == 6);

    // 初回構築と墓石の置換で引数変換が失敗しても、登録状態を変えない。
    HashMap<int, std::int64_t, StorageCollision> failures;
    conversion_throws([&] { failures.try_emplace(1, ThrowScalar{true}); });
    assert(failures.empty() && !failures.contains(1));
    failures[1] = 31;
    failures[2] = 32;
    failures.erase(1, false);
    conversion_throws([&] { failures.try_emplace(1, ThrowScalar{true}); });
    conversion_throws([&] { failures.try_emplace(3, ThrowScalar{true}); });
    assert(failures.size() == 1 && failures.at(2) == 32 && !failures.contains(1) && !failures.contains(3));
    auto failed_copy = failures;
    failed_copy.try_emplace(1, ThrowScalar{false});
    assert(failed_copy.at(1) == 73 && failed_copy.at(2) == 32 && !failures.contains(1));

    // アラインメントと、未登録時の値初期化を型ごとに確認する。
    HashMap<int, AlignedValue> aligned;
    for (int key = 0; key < 80; ++key) {
        auto& value = aligned[key];
        for (auto word : value.words) assert(word == 0);
        value.words[7] = key + 1;
        assert(reinterpret_cast<std::uintptr_t>(std::addressof(value)) % alignof(AlignedValue) == 0);
    }
    auto aligned_copy = aligned;
    aligned_copy.rehash(1000);
    for (int key = 0; key < 80; ++key) assert(aligned_copy.at(key).words[7] == key + 1);
    HashMap<int, int*> pointers;
    assert(pointers[1] == nullptr);
    int value = 77;
    pointers[1] = &value;
    auto pointer_copy = pointers;
    assert(pointer_copy.at(1) == &value && pointer_copy[2] == nullptr);
    std::cout << "PASS sparse/inactive/tombstone copy, nontrivial move, throwing conversion, alignment/default values\n";
}
