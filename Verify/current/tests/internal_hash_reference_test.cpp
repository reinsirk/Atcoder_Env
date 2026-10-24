// SPDX-License-Identifier: CC0-1.0
// 独立したstd::map参照で整数キー表と所有操作を検査する。
#ifndef SIRKIT_INTERNAL_HASH_SOURCE
#define SIRKIT_INTERNAL_HASH_SOURCE "../../../Internal/Nyaan_hash_map_min.cpp"
#endif
#include SIRKIT_INTERNAL_HASH_SOURCE
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

static std::uint64_t checks = 0;
static void require(bool ok) { ++checks; if (!ok) std::abort(); }

template<class Table, class Key, class Value>
static void inspect(const Table& table, const std::map<Key, Value>& ref) {
    require(table.size() == ref.size() && table.empty() == ref.empty());
    auto got = table.enumerate();
    std::sort(got.begin(), got.end());
    const std::vector<std::pair<Key, Value>> want(ref.begin(), ref.end());
    require(got == want);
    std::map<Key, Value> visited;
    table.for_each([&](const Key& key, const Value& value) {
        require(visited.emplace(key, value).second);
        require(table.find(key) && *table.find(key) == value);
        require(table.find_nonempty(key) && *table.find_nonempty(key) == value);
    });
    require(visited == ref);
}

template<class Key, bool Packed, unsigned D>
static void run_numeric(int operations) {
    using Table = reinsirk::detail::InternalHashMap<Key, std::int64_t, Packed, D>;
    Table table;
    std::map<Key, std::int64_t> ref;
    std::int64_t initial = -19;
    table.set_default(initial);
    std::mt19937_64 rng(0xD1509B + sizeof(Key) * 71 + D * 11 + unsigned(Packed));
    for (Key key : {std::numeric_limits<Key>::min(), std::numeric_limits<Key>::max(), Key{0}, Key{1}}) {
        require(table[key] == (ref.try_emplace(key, initial).first->second));
    }
    for (int rep = 0; rep < operations; ++rep) {
        Key key = static_cast<Key>(rng());
        if (rep % 5 == 0) key = static_cast<Key>(rng() % 128);
        switch (rng() % 12) {
        case 0: case 1: {
            const auto value = static_cast<std::int64_t>(rng() % 2000001) - 1000000;
            table[key] = value; ref[key] = value; break;
        }
        case 2: {
            const auto old = ref.find(key);
            require(table[key] == (old == ref.end() ? initial : old->second));
            ref.try_emplace(key, initial); break;
        }
        case 3: {
            const auto old = ref.find(key);
            int calls = 0;
            const auto value = static_cast<std::int64_t>(rng() % 101);
            auto& got = table.find_or_emplace(key, [&] { ++calls; return value; });
            require(calls == int(old == ref.end()));
            require(got == (old == ref.end() ? value : old->second));
            require(&got == table.find(key));
            ref.try_emplace(key, value); break;
        }
        case 4: {
            const auto old = ref.find(key); const auto* got = table.find(key);
            require((got == nullptr) == (old == ref.end()));
            require(table.contains(key) == (old != ref.end()));
            require(table.contain(key) == table.contains(key));
            require(table.count(key) == int(old != ref.end()));
            if (got) { require(*got == old->second); *table.find(key) += 1; ++ref[key]; }
            break;
        }
        case 5: if (rep % 97 == 0) { table.clear(); ref.clear(); } break;
        case 6: initial = static_cast<std::int64_t>(rng()%1000)-500; table.set_default(initial); break;
        case 7: if (rep % 37 == 0) table.reserve(ref.size() + 101); break;
        case 8: if (rep % 41 == 0) {
            auto copy = table;
            inspect(copy, ref);
            copy.reallocate(); inspect(copy, ref);
            auto moved = std::move(copy); require(copy.empty()); inspect(moved, ref);
            copy.set_default(83); copy[Key{0}] = 79;
            require(copy.size() == 1 && copy[Key{0}] == 79);
            copy = table; inspect(copy, ref);
            Table assigned; assigned = std::move(moved); require(moved.empty()); inspect(assigned, ref);
            break;
        } break;
        case 9: if (rep % 43 == 0) {
            Table other; other.set_default(77); other[Key{0}] = 71;
            table.swap(other); require(table.size() == 1 && table[Key{0}] == 71);
            table.swap(other); inspect(table, ref);
            break;
        } break;
        case 10: if (rep % 101 == 0) inspect(table, ref); break;
        default: break;
        }
    }
    inspect(table, ref);
    bool overflow = false;
    try { table.reserve(std::numeric_limits<std::size_t>::max()); }
    catch (const std::length_error&) { overflow = true; }
    require(overflow); inspect(table, ref);
    table.clear(); require(table.empty() && table.find(Key{0}) == nullptr);
    // clear後も容量を持つため、find_nonemptyの前提を満たす。
    require(table.find_nonempty(Key{0}) == nullptr);
}

template<class Key, bool Packed>
static void denominators() {
    run_numeric<Key, Packed, 4>(1000);
    run_numeric<Key, Packed, 5>(1000);
}
template<bool Packed> static void additional_denominators() {
    run_numeric<std::uint64_t, Packed, 2>(1000);
    run_numeric<std::uint64_t, Packed, 3>(1000);
    run_numeric<std::uint64_t, Packed, 6>(1000);
    run_numeric<std::uint64_t, Packed, 7>(1000);
    run_numeric<std::uint64_t, Packed, 8>(1000);
}

template<bool Packed, unsigned D>
static void stable_reserved_references() {
    using Table = reinsirk::detail::InternalHashMap<std::uint64_t, std::int64_t, Packed, D>;
    Table table;
    table.reserve(5000);
    std::vector<const std::int64_t*> pointers;
    for (std::uint64_t i = 0; i < 1000; ++i) pointers.push_back(&(table[i] = std::int64_t(i * 13)));
    for (std::uint64_t i = 1000; i < 5000; ++i) table[i] = std::int64_t(i * 13);
    for (std::uint64_t i = 0; i < pointers.size(); ++i)
        require(pointers[std::size_t(i)] == table.find(i) && *pointers[std::size_t(i)] == std::int64_t(i * 13));
    table.reserve(5000);
    for (std::uint64_t i = 0; i < pointers.size(); ++i) require(pointers[std::size_t(i)] == table.find(i));
}

template<bool Packed>
static void owned_values() {
    using StringTable = reinsirk::detail::InternalHashMap<int, std::string, Packed>;
    StringTable table; table.set_default("new-value");
    std::map<int, std::string> ref;
    for (int i = -256; i <= 256; ++i) {
        require(table[i] == "new-value");
        table[i] = std::to_string(i); ref[i] = std::to_string(i);
    }
    inspect(table, ref);
    auto copy = table; copy[3] = "independent";
    require(table[3] == "3" && copy[3] == "independent");
    table.clear(); ref.clear(); table.set_default("after-clear");
    require(table[999] == "after-clear"); ref[999] = "after-clear"; inspect(table, ref);
    std::weak_ptr<int> weak;
    {
        reinsirk::detail::InternalHashMap<int, std::shared_ptr<int>, Packed> owners;
        auto value = std::make_shared<int>(42); weak = value;
        owners[11] = value; value.reset();
        require(!weak.expired() && **owners.find(11) == 42);
        owners.clear(); require(owners.empty() && !owners.find(11));
        // clearの後も未登録スロット内のValを保持する現行契約。
        require(!weak.expired());
        auto duplicate = owners; require(duplicate.empty());
    }
    require(weak.expired());
}

int main() {
    denominators<std::int8_t, false>(); denominators<std::int8_t, true>();
    denominators<std::uint8_t, false>(); denominators<std::uint8_t, true>();
    denominators<std::int16_t, false>(); denominators<std::int16_t, true>();
    denominators<std::uint16_t, false>(); denominators<std::uint16_t, true>();
    denominators<std::int32_t, false>(); denominators<std::int32_t, true>();
    denominators<std::uint32_t, false>(); denominators<std::uint32_t, true>();
    denominators<std::int64_t, false>(); denominators<std::int64_t, true>();
    denominators<std::uint64_t, false>(); denominators<std::uint64_t, true>();
    additional_denominators<false>(); additional_denominators<true>();
    run_numeric<std::int64_t, false, 5>(50000); run_numeric<std::uint64_t, true, 4>(50000);
    stable_reserved_references<false, 2>(); stable_reserved_references<false, 5>();
    stable_reserved_references<true, 4>(); stable_reserved_references<true, 8>();
    owned_values<false>(); owned_values<true>();
    std::cout << "PASS " << checks << " independent InternalHashMap checks, 142000 random operations\n";
}
