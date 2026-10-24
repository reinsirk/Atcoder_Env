// SPDX-License-Identifier: CC0-1.0
// 非自明な値のコピー失敗・再配置失敗後も登録済み内容を保つ。
#ifndef SIRKIT_INTERNAL_HASH_SOURCE
#define SIRKIT_INTERNAL_HASH_SOURCE "../../../Internal/Nyaan_hash_map_min.cpp"
#endif
#include SIRKIT_INTERNAL_HASH_SOURCE
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

static std::uint64_t checks = 0;
static void require(bool ok) { ++checks; if (!ok) std::abort(); }
struct Fault {};
struct Bomb {
    static inline int copies = -1, assignments = -1;
    std::string value;
    Bomb() = default;
    explicit Bomb(std::string s) : value(std::move(s)) {}
    Bomb(const Bomb& other) : value(other.value) {
        if (copies == 0) throw Fault{};
        if (copies > 0) --copies;
    }
    Bomb(Bomb&&) noexcept = default;
    Bomb& operator=(const Bomb& other) {
        if (assignments == 0) { value = "poison"; throw Fault{}; }
        if (assignments > 0) --assignments;
        value = other.value; return *this;
    }
    Bomb& operator=(Bomb&&) noexcept = default;
    friend void swap(Bomb& a, Bomb& b) noexcept { a.value.swap(b.value); }
};

template<class Table>
static void inspect(const Table& table, const std::map<int, std::string>& ref) {
    require(table.size() == ref.size());
    for (const auto& [key, value] : ref) require(table.find(key) && table.find(key)->value == value);
    std::map<int, std::string> values;
    table.for_each([&](int key, const Bomb& value) { require(values.emplace(key, value.value).second); });
    require(values == ref);
}

template<bool Packed, unsigned D>
static void copy_and_assignment() {
    using Table = reinsirk::detail::InternalHashMap<int, Bomb, Packed, D>;
    Table source, destination;
    std::map<int, std::string> ref, old;
    for (int i = 0; i < 31; ++i) { source.find_or_emplace(i, [i] { return Bomb(std::to_string(i)); }); ref[i] = std::to_string(i); }
    for (int i = 50; i < 57; ++i) { destination.find_or_emplace(i, [i] { return Bomb(std::to_string(i)); }); old[i] = std::to_string(i); }
    for (int budget : {0, 1, 5, 17, 31}) {
        bool failed = false; Bomb::copies = budget;
        try { auto copy = source; (void)copy; } catch (Fault) { failed = true; }
        Bomb::copies = -1; require(failed); inspect(source, ref);
        Bomb::copies = budget; failed = false;
        try { destination = source; } catch (Fault) { failed = true; }
        Bomb::copies = -1; require(failed); inspect(destination, old); inspect(source, ref);
    }
    destination = source; inspect(destination, ref);
    destination.find(0)->value = "independent"; require(source.find(0)->value == "0");
    // 再配置のコピー代入先が変更されてから失敗しても、元の表は不変。
    for (int budget : {0, 1, 5, 17, 30}) {
        bool failed = false; Bomb::assignments = budget;
        try { source.reserve(1024); } catch (Fault) { failed = true; }
        Bomb::assignments = -1; require(failed); inspect(source, ref);
    }
    source.reserve(1024); inspect(source, ref);
    bool failed = false; Bomb::assignments = 0;
    try { (void)source[999]; } catch (Fault) { failed = true; }
    Bomb::assignments = -1; require(failed && !source.find(999)); inspect(source, ref);
    // 例外後に同じ未公開スロットを正常に使える。
    source.find_or_emplace(999, [] { return Bomb("recovered"); }); ref[999] = "recovered"; inspect(source, ref);
    failed = false;
    try { source.for_each([](int, const Bomb&) { throw Fault{}; }); } catch (Fault) { failed = true; }
    require(failed); inspect(source, ref);
    Bomb::copies = 0; failed = false;
    try { (void)source.enumerate(); } catch (Fault) { failed = true; }
    Bomb::copies = -1; require(failed); inspect(source, ref);
    // set_default失敗時は初期値の不変を保証しないが、既存登録は変化しない。
    Bomb::assignments = 0; failed = false;
    try { source.set_default(Bomb("new-default")); } catch (Fault) { failed = true; }
    Bomb::assignments = -1; require(failed); inspect(source, ref);
    source.set_default(Bomb("restored-default"));
    require(source[1000].value == "restored-default");
}

struct SwapFaultValue {
    static inline bool armed = false;
    int value = 0;
    friend void swap(SwapFaultValue& a, SwapFaultValue& b) {
        if (armed) { a.value = -999; throw Fault{}; }
        std::swap(a.value, b.value);
    }
};
static void throwing_default_swap() {
    using Table = reinsirk::detail::InternalHashMap<int, SwapFaultValue>;
    Table a, b; a[1].value = 17; b[2].value = 29;
    SwapFaultValue::armed = true; bool failed = false;
    try { a.swap(b); } catch (Fault) { failed = true; }
    SwapFaultValue::armed = false;
    require(failed && a.size() == 1 && b.size() == 1);
    require(a.find(1)->value == 17 && !a.find(2) && b.find(2)->value == 29 && !b.find(1));
    a.set_default(SwapFaultValue{42}); b.set_default(SwapFaultValue{53});
    require(a[3].value == 42 && b[4].value == 53);
    a.swap(b); require(a.find(2)->value == 29 && b.find(1)->value == 17);
}
int main() {
    copy_and_assignment<false, 2>(); copy_and_assignment<false, 5>();
    copy_and_assignment<true, 4>(); copy_and_assignment<true, 8>();
    throwing_default_swap();
    std::cout << "PASS " << checks << " nontrivial value/exception checks\n";
}
