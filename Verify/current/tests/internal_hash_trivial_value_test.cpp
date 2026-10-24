// SPDX-License-Identifier: CC0-1.0
// 自明な構造体・配列・非所有ポインタ値でも値の生存期間とコピーを検査する。
#ifndef SIRKIT_INTERNAL_HASH_SOURCE
#define SIRKIT_INTERNAL_HASH_SOURCE "../../../Internal/Nyaan_hash_map_min.cpp"
#endif
#include SIRKIT_INTERNAL_HASH_SOURCE
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <type_traits>
struct Pair { std::int32_t a, b; };
static_assert(std::is_trivial_v<Pair>);
static std::uint64_t checks = 0;
static void require(bool ok) { ++checks; if (!ok) std::abort(); }

template<bool Packed, unsigned D> static void run() {
    reinsirk::detail::InternalHashMap<std::int64_t, Pair, Packed, D> pairs;
    pairs.set_default({17, 29});
    for (std::int64_t key = -1000; key <= 1000; ++key) {
        auto& item = pairs[key]; require(item.a == 17 && item.b == 29);
        item = {std::int32_t(key), std::int32_t(key + 71)};
    }
    auto copy = pairs; pairs.clear();
    for (std::int64_t key = -1000; key <= 1000; ++key) {
        const auto* item = copy.find(key); require(item && item->a == key && item->b == key + 71);
    }
    require(pairs[-1000].a == 17 && pairs[-1000].b == 29);
    copy.reserve(9000);
    for (std::int64_t key = -1000; key <= 1000; ++key) require(copy.find(key)->a == key);
    using Array = std::array<std::uint16_t, 2>;
    static_assert(std::is_trivial_v<Array>);
    reinsirk::detail::InternalHashMap<std::uint32_t, Array, Packed, D> arrays;
    arrays.set_default({31, 37});
    for (std::uint32_t key = 0; key < 1000; ++key) {
        require((arrays[key] == Array{31,37})); arrays[key] = {std::uint16_t(key), std::uint16_t(key+7)};
    }
    auto array_copy = arrays; arrays.clear();
    for (std::uint32_t key = 0; key < 1000; ++key) require(((*array_copy.find(key)) == Array{std::uint16_t(key), std::uint16_t(key+7)}));
    std::array<std::int64_t, 1000> storage{};
    reinsirk::detail::InternalHashMap<std::uint64_t, std::int64_t*, Packed, D> pointers;
    pointers.set_default(nullptr);
    for (std::uint64_t key = 0; key < storage.size(); ++key) {
        require(pointers[key] == nullptr); storage[std::size_t(key)] = std::int64_t(key); pointers[key] = &storage[std::size_t(key)];
    }
    auto pointer_copy = pointers; pointers.clear();
    for (std::uint64_t key = 0; key < storage.size(); ++key) require(**pointer_copy.find(key) == std::int64_t(key));
}
int main() {
    run<false,2>(); run<false,5>(); run<true,4>(); run<true,8>();
    std::cout << "PASS " << checks << " trivial class/array/non-owning pointer value checks\n";
}
