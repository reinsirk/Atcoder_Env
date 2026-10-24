// SPDX-License-Identifier: CC0-1.0
#ifndef HEADER
#define HEADER "../../../DataStructure/Hash_set.cpp"
#endif
#include HEADER
#include <cassert>
#include <cstddef>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

struct ThrowingStringHash {
    static inline bool armed = false;
    std::size_t operator()(const std::string& value) const {
        if (armed && value.starts_with("key-17#")) throw std::runtime_error("string hash");
        return GenericHash(83)(value);
    }
};
struct OwnedText {
    std::string value;
    explicit OwnedText(std::string text) : value(std::move(text)) {}
    bool operator==(const OwnedText&) const = default;
};
namespace std {
template <> struct hash<OwnedText> {
    static inline bool armed = false;
    std::size_t operator()(const OwnedText& value) const {
        if (armed && value.value.starts_with("key-17#")) throw std::runtime_error("user hash");
        return std::hash<std::string>{}(value.value);
    }
};
}

template <class Action> void hash_throws(Action action) {
    bool caught = false;
    try { action(); } catch (const std::runtime_error&) { caught = true; }
    assert(caught);
}

template <class Table>
void verify_strings(const Table& table, const std::set<std::string>& reference) {
    assert(table.size() == reference.size());
    for (const auto& key : reference) assert(table.contains(key));
    std::set<std::string> enumerated(table.begin(), table.end());
    assert(enumerated == reference);
}

int main() {
    static_assert(noexcept(std::hash<std::string>{}(std::declval<const std::string&>())));
    static_assert(std::is_nothrow_move_constructible_v<std::string>);
    static_assert(!std::is_nothrow_move_constructible_v<const std::string>);
    std::vector<std::string> keys{"", std::string("a\0b", 3), "short", std::string(2048, 'z')};
    for (int i = 0; i < 2500; ++i)
        keys.push_back("key-" + std::to_string(i) + "#" + std::string(80, char('a' + i % 26)));
    const std::set<std::string> reference(keys.begin(), keys.end());
    HashSet<std::string> table(0, GenericHash(97));
    for (const auto& key : keys) assert(table.insert_checked(key).second);
    for (const auto& key : keys) assert(!table.insert_checked(key).second);
    verify_strings(table, reference);
    table.reserve(12000);
    verify_strings(table, reference);
    table.rehash(8);
    verify_strings(table, reference);
    table.erase(keys[1], false);
    table.insert(keys[1]);
    table.rehash(table.capacity());
    verify_strings(table, reference);
    auto copy = table;
    copy.erase(keys.back(), false);
    assert(table.contains(keys.back()) && !copy.contains(keys.back()));
    copy = table;
    auto moved = std::move(copy);
    assert(copy.empty());
    verify_strings(moved, reference);
    moved.clear();
    assert(moved.empty());
    moved.insert(keys.back());
    assert(moved.size() == 1 && moved.contains(keys.back()));
    verify_strings(table, reference);

    // const文字列はコピーが必要なので、例外を考慮した従来経路に残す。
    HashSet<const std::string> constant(0, GenericHash(101));
    for (const auto& key : keys) constant.insert(key);
    constant.reserve(12000);
    constant.rehash(8);
    verify_strings(constant, reference);
    auto constant_copy = constant;
    constant_copy.clear();
    verify_strings(constant, reference);

    // 任意のHasherは非送出と仮定せず、失敗後も長い文字列を保持する。
    HashSet<std::string, ThrowingStringHash> custom;
    for (const auto& key : keys) custom.insert(key);
    const auto old_capacity = custom.capacity();
    ThrowingStringHash::armed = true;
    hash_throws([&] { custom.reserve(12000); });
    hash_throws([&] { custom.rehash(old_capacity); });
    ThrowingStringHash::armed = false;
    assert(custom.capacity() == old_capacity);
    verify_strings(custom, reference);

    // GenericHashでも、利用者定義型のstd::hashは例外を送出しうる。
    HashSet<OwnedText> owned;
    for (const auto& key : keys) owned.emplace(key);
    const auto owned_capacity = owned.capacity();
    std::hash<OwnedText>::armed = true;
    hash_throws([&] { owned.reserve(12000); });
    hash_throws([&] { owned.rehash(owned_capacity); });
    std::hash<OwnedText>::armed = false;
    assert(owned.capacity() == owned_capacity && owned.size() == reference.size());
    for (const auto& key : keys) assert(owned.contains(OwnedText(key)));
    std::cout << "PASS long/short/NUL strings, const keys, copy/move/erase, throwing custom and user-defined hashes\n";
}
