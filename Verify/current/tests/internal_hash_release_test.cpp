// SPDX-License-Identifier: CC0-1.0
// capacity/load_factor/releaseの解放・初期値保持と再利用を検査する。
#ifndef SIRKIT_INTERNAL_HASH_SOURCE
#define SIRKIT_INTERNAL_HASH_SOURCE "../../../Internal/Nyaan_hash_map_min.cpp"
#endif
#include SIRKIT_INTERNAL_HASH_SOURCE
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>
#include <utility>

static bool deny_allocation = false;
static std::size_t live_allocations = 0, checks = 0;
[[gnu::noinline]] void* operator new(std::size_t n) {
    if (deny_allocation) throw std::bad_alloc();
    if (void* p = std::malloc(n ? n : 1)) { ++live_allocations; return p; }
    throw std::bad_alloc();
}
[[gnu::noinline]] void* operator new[](std::size_t n) { return ::operator new(n); }
[[gnu::noinline]] static void free_block(void* p) noexcept { if (p) --live_allocations; std::free(p); }
void operator delete(void* p) noexcept { free_block(p); }
void operator delete[](void* p) noexcept { free_block(p); }
void operator delete(void* p, std::size_t) noexcept { free_block(p); }
void operator delete[](void* p, std::size_t) noexcept { free_block(p); }
static void require(bool ok) { ++checks; if (!ok) std::abort(); }

template<bool Packed, unsigned D>
static void numeric() {
    using Map = reinsirk::detail::InternalHashMap<std::uint64_t, std::int64_t, Packed, D>;
    const auto before = live_allocations;
    {
        Map table;
        require(table.empty() && table.capacity() == 0 && table.load_factor() == 0.0);
        deny_allocation = true; table.release(); table.release(); deny_allocation = false;
        require(live_allocations == before);
        table.set_default(73); table.reserve(1000);
        const auto capacity = table.capacity();
        require(std::has_single_bit(capacity) && capacity - capacity / D >= 1000);
        for (std::uint64_t i = 0; i < 1000; ++i) require(table[i] == 73);
        require(table.load_factor() == double(table.size()) / double(capacity));
        table.clear(); require(table.capacity() == capacity && table.empty() && table.load_factor() == 0);
        require(live_allocations > before);
        deny_allocation = true; table.release(); deny_allocation = false;
        require(table.empty() && table.capacity() == 0 && table.load_factor() == 0);
        require(table.find(0) == nullptr && live_allocations == before);
        require(table[0] == 73); table[0] = 91;
        Map moved(std::move(table));
        require(table.empty() && table.capacity() == 0 && moved[0] == 91);
        deny_allocation = true; table.release(); deny_allocation = false;
        moved.release(); require(moved.empty() && moved.capacity() == 0 && live_allocations == before);
        require(moved[1] == 73);
        moved.release();
        table.set_default(81); require(table[2] == 81); table.release();
        require(live_allocations == before);
    }
    require(live_allocations == before);
}

template<bool Packed>
static void resources() {
    std::weak_ptr<int> default_weak, item_weak;
    {
        reinsirk::detail::InternalHashMap<int, std::shared_ptr<int>, Packed> table;
        auto first = std::make_shared<int>(17), second = std::make_shared<int>(29);
        default_weak = first; item_weak = second;
        table.set_default(first); table[1] = second; first.reset(); second.reset();
        require(!default_weak.expired() && !item_weak.expired());
        table.clear(); require(table.empty() && !item_weak.expired());
        deny_allocation = true; table.release(); deny_allocation = false;
        require(table.capacity() == 0 && item_weak.expired() && !default_weak.expired());
        require(table[2] && *table[2] == 17);
        table.release(); require(!default_weak.expired());
        table.set_default(std::shared_ptr<int>{}); require(default_weak.expired());
    }
    require(default_weak.expired() && item_weak.expired());
}
int main() {
    const auto before = live_allocations;
    numeric<false, 2>(); numeric<false, 3>(); numeric<false, 4>(); numeric<false, 5>();
    numeric<false, 6>(); numeric<false, 7>(); numeric<false, 8>();
    numeric<true, 2>(); numeric<true, 3>(); numeric<true, 4>(); numeric<true, 5>();
    numeric<true, 6>(); numeric<true, 7>(); numeric<true, 8>();
    resources<false>(); resources<true>();
    require(live_allocations == before);
    std::printf("PASS %zu capacity/load_factor/release checks; no retained array allocation\n", checks);
}
