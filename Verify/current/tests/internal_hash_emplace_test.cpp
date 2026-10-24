// SPDX-License-Identifier: CC0-1.0
// find_or_emplaceの遅延生成、公開タイミング、確保失敗後の内容を検査する。
#ifndef INTERNAL_HASH_SOURCE
#define INTERNAL_HASH_SOURCE "../../../Internal/Nyaan_hash_map_min.cpp"
#endif
#include INTERNAL_HASH_SOURCE
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <new>
#include <source_location>
#include <utility>
#include <vector>

static int allocation_budget = -1;
static std::size_t allocations = 0, live_allocations = 0;
static std::size_t checks = 0, failure_points = 0;
[[gnu::noinline]] void* operator new(std::size_t bytes) {
    if (allocation_budget == 0) throw std::bad_alloc();
    if (allocation_budget > 0) --allocation_budget;
    if (void* p = std::malloc(bytes ? bytes : 1)) {
        ++allocations;
        ++live_allocations;
        return p;
    }
    throw std::bad_alloc();
}
[[gnu::noinline]] void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
[[gnu::noinline]] static void release(void* p) noexcept {
    if (p) --live_allocations;
    std::free(p);
}
void operator delete(void* p) noexcept { release(p); }
void operator delete[](void* p) noexcept { release(p); }
void operator delete(void* p, std::size_t) noexcept { release(p); }
void operator delete[](void* p, std::size_t) noexcept { release(p); }
static void require(bool condition,
                    std::source_location loc = std::source_location::current()) {
    ++checks;
    if (!condition) {
        std::fprintf(stderr, "failed line %u\n", loc.line());
        std::abort();
    }
}
struct FactoryException {};
struct AssignmentException {};

struct ThrowValue {
    static inline int assignment_budget = -1;
    int value = 0;
    ThrowValue() = default;
    explicit ThrowValue(int v) : value(v) {}
    ThrowValue(const ThrowValue&) = default;
    ThrowValue(ThrowValue&&) = default;
    ThrowValue& operator=(const ThrowValue& other) {
        if (assignment_budget == 0) {
            value = -999;
            throw AssignmentException{};
        }
        if (assignment_budget > 0) --assignment_budget;
        value = other.value;
        return *this;
    }
    ThrowValue& operator=(ThrowValue&& other) {
        return *this = static_cast<const ThrowValue&>(other);
    }
};

struct RvalueFactory {
    int* calls;
    int value;
    int operator()() && { ++*calls; return value; }
};

[[gnu::noinline]] static int vector_value(const std::vector<int>& values) {
    volatile int first = values[0];
    return first + int(values.size());
}

template<bool Packed, unsigned Denominator>
static void ordinary_and_factory_exceptions() {
    using Table = reinsirk::detail::InternalHashMap<int, int, Packed, Denominator>;
    Table table;
    table.set_default(12345);
    require(table.empty() && table.size() == 0);
    int calls = 0;
    bool caught = false;
    try {
        table.find_or_emplace(std::numeric_limits<int>::min(), [&]() -> int {
            ++calls;
            throw FactoryException{};
        });
    } catch (FactoryException) { caught = true; }
    require(caught && calls == 1 && table.empty());
    require(!table.contains(std::numeric_limits<int>::min()));
    auto& first = table.find_or_emplace(std::numeric_limits<int>::min(), RvalueFactory{&calls, 7});
    require(first == 7 && calls == 2);
    require(&first == table.find(std::numeric_limits<int>::min()));
    const auto before_allocations = allocations;
    allocation_budget = 0;
    auto& existing = table.find_or_emplace(std::numeric_limits<int>::min(), [&]() -> int {
        ++calls;
        throw FactoryException{};
    });
    allocation_budget = -1;
    require(&existing == &first && calls == 2 && allocations == before_allocations);
    first = -8;
    require(*table.find(std::numeric_limits<int>::min()) == -8);
    table.reserve(64);
    std::map<int, int> expected{{std::numeric_limits<int>::min(), -8}};
    for (int key : {-99, -1, 0, 1, 17, std::numeric_limits<int>::max()}) {
        const int value = key == std::numeric_limits<int>::max() ? 99 : key * 3;
        expected[key] = value;
        auto& slot = table.find_or_emplace(key, [value] { return value; });
        require(slot == value && &slot == table.find(key));
    }
    auto snapshot = table.enumerate();
    std::sort(snapshot.begin(), snapshot.end());
    caught = false;
    try {
        table.find_or_emplace(-12345, [&]() -> int { ++calls; throw FactoryException{}; });
    } catch (FactoryException) { caught = true; }
    require(caught && !table.contains(-12345) && table.size() == expected.size());
    auto current = table.enumerate();
    std::sort(current.begin(), current.end());
    require(current == snapshot);
    require(table.find_or_emplace(-12345, [] { return 33; }) == 33);
    table.clear();
    require(table.empty() && !table.contains(-12345));
    require(table.find_or_emplace(-12345, [] { return -7; }) == -7);

    // 他のdynamic系でも使う64ビットキーを切り詰めず区別する。
    reinsirk::detail::InternalHashMap<std::int64_t, int, Packed, Denominator> wide;
    int value = 0;
    for (std::int64_t key : {std::numeric_limits<std::int64_t>::min(), std::int64_t{-1},
                             std::int64_t{0}, std::int64_t{1},
                             std::numeric_limits<std::int64_t>::max()}) {
        require(wide.find_or_emplace(key, [value] { return value; }) == value);
        require(wide.find(key) && *wide.find(key) == value);
        ++value;
    }
    require(wide.size() == 5);
}

template<bool Packed, unsigned Denominator>
static void allocation_failures() {
    using Table = reinsirk::detail::InternalHashMap<int, int, Packed, Denominator>;
    const int threshold = 8 - 8 / int(Denominator);
    for (bool filled : {false, true}) {
        bool reached_success = false;
        int failures_before_factory = 0, failures_in_factory = 0;
        for (int budget = 0; budget != 7; ++budget) {
            Table table;
            if (filled)
                for (int i = 0; i < threshold; ++i)
                    table.find_or_emplace(i, [i] { return i * 19; });
            const auto size_before = table.size();
            int factory_calls = 0;
            bool failed = false;
            allocation_budget = budget;
            try {
                const int result = table.find_or_emplace(-7654321, [&] {
                    ++factory_calls;
                    std::vector<int> temporary(128, 23);
                    return vector_value(temporary);
                });
                require(result == 151);
            } catch (const std::bad_alloc&) { failed = true; }
            allocation_budget = -1;
            if (failed) {
                ++failure_points;
                require(table.size() == size_before && !table.contains(-7654321));
                if (factory_calls == 0) ++failures_before_factory;
                else { require(factory_calls == 1); ++failures_in_factory; }
                if (filled)
                    for (int i = 0; i < threshold; ++i)
                        require(table.find(i) && *table.find(i) == i * 19);
                require(table.find_or_emplace(-7654321, [] { return 41; }) == 41);
                require(table.size() == size_before + 1);
            } else {
                reached_success = true;
                require(factory_calls == 1 && table.size() == size_before + 1);
                break;
            }
        }
        require(reached_success && failures_before_factory >= 1 && failures_in_factory >= 1);
    }
}

template<bool Packed, unsigned Denominator>
static void value_assignment_and_rehash_exceptions() {
    using Table = reinsirk::detail::InternalHashMap<int, ThrowValue, Packed, Denominator>;
    const int threshold = 8 - 8 / int(Denominator);
    Table table;
    for (int i = 0; i < threshold; ++i)
        table.find_or_emplace(i, [i] { return ThrowValue(i + 100); });
    int factory_calls = 0;
    bool caught = false;
    // まず再配置中のコピー代入を失敗させる。factoryにはまだ到達しない。
    ThrowValue::assignment_budget = 0;
    try {
        table.find_or_emplace(-1, [&] { ++factory_calls; return ThrowValue(7); });
    } catch (AssignmentException) { caught = true; }
    ThrowValue::assignment_budget = -1;
    require(caught && factory_calls == 0 && table.size() == std::size_t(threshold));
    require(!table.contains(-1));
    for (int i = 0; i < threshold; ++i) require(table.find(i)->value == i + 100);
    table.reserve(32);
    // コピー先の未公開スロットを変更してから例外にしても、登録されないこと。
    caught = false;
    ThrowValue::assignment_budget = 0;
    try {
        table.find_or_emplace(-1, [&] { ++factory_calls; return ThrowValue(7); });
    } catch (AssignmentException) { caught = true; }
    ThrowValue::assignment_budget = -1;
    require(caught && factory_calls == 1 && table.size() == std::size_t(threshold));
    require(!table.contains(-1));
    for (int i = 0; i < threshold; ++i) require(table.find(i)->value == i + 100);
    require(table.find_or_emplace(-1, [&] { ++factory_calls; return ThrowValue(31); }).value == 31);
    require(table.size() == std::size_t(threshold + 1));
    const auto before_allocations = allocations;
    allocation_budget = 0;
    ThrowValue::assignment_budget = 0;
    auto& slot = table.find_or_emplace(-1, [&]() -> ThrowValue { throw FactoryException{}; });
    require(slot.value == 31);
    ThrowValue::assignment_budget = -1;
    allocation_budget = -1;
    require(allocations == before_allocations);
}

template<bool Packed, unsigned Denominator>
static void run() {
    ordinary_and_factory_exceptions<Packed, Denominator>();
    allocation_failures<Packed, Denominator>();
    value_assignment_and_rehash_exceptions<Packed, Denominator>();
}

int main() {
    const auto before_live = live_allocations;
    run<false, 4>();
    run<false, 5>();
    run<true, 4>();
    run<true, 5>();
    require(live_allocations == before_live);
    std::printf("{\"status\":\"PASS\",\"checks\":%zu,\"allocation_failure_points\":%zu,\"live_delta\":%zd}\n",
                checks, failure_points, static_cast<std::ptrdiff_t>(live_allocations) - static_cast<std::ptrdiff_t>(before_live));
}
