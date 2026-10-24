// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_RADIX_SOURCE
#define SIRKIT_RADIX_SOURCE "../../../Algorithm/Radix_sort.cpp"
#endif
#include SIRKIT_RADIX_SOURCE
#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <new>
#include <numeric>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
std::ptrdiff_t remaining = -1;
std::size_t live = 0, checks = 0, allocation_failures = 0;
}
[[gnu::noinline]] void* operator new(std::size_t bytes) {
    if (remaining == 0) throw std::bad_alloc();
    if (remaining > 0) --remaining;
    if (void* p = std::malloc(bytes ? bytes : 1)) { ++live; return p; }
    throw std::bad_alloc();
}
[[gnu::noinline]] void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
[[gnu::noinline]] void* operator new(std::size_t bytes, const std::nothrow_t&) noexcept {
    try { return ::operator new(bytes); }
    catch (const std::bad_alloc&) { return nullptr; }
}
[[gnu::noinline]] void* operator new[](std::size_t bytes, const std::nothrow_t& tag) noexcept {
    return ::operator new(bytes, tag);
}
[[gnu::noinline]] static void free_block(void* p) noexcept {
    if (p) --live;
    std::free(p);
}
void operator delete(void* p) noexcept { free_block(p); }
void operator delete[](void* p) noexcept { free_block(p); }
void operator delete(void* p, std::size_t) noexcept { free_block(p); }
void operator delete[](void* p, std::size_t) noexcept { free_block(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { free_block(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { free_block(p); }

static void require_impl(bool ok, const char* expression, int line) {
    ++checks;
    if (!ok) {
        std::fprintf(stderr, "line %d: %s (%ju checks)\n", line, expression,
                     static_cast<std::uintmax_t>(checks));
        std::abort();
    }
}
#define SIRKIT_TEST_REQUIRE(ok) require_impl(bool(ok), #ok, __LINE__)

template<class Function> static bool fails_with_budget(int budget, Function function) {
    bool failed = false;
    remaining = budget;
    try { function(); }
    catch (const std::bad_alloc&) { failed = true; ++allocation_failures; }
    remaining = -1;
    return failed;
}

template<class T> static std::vector<T> make_values(std::size_t n) {
    using U = std::make_unsigned_t<T>;
    std::mt19937_64 generator(923491);
    std::vector<T> values(n);
    for (std::size_t i = 0; i < n; ++i) {
        U bits = static_cast<U>(generator());
        if (i % 3) bits &= U{63};
        values[i] = std::bit_cast<T>(bits);
    }
    return values;
}

template<class T> static void inspect_values(const std::vector<T>& original,
                                            RadixSortWorkspace<T>& workspace) {
    auto expected = original;
    std::stable_sort(expected.begin(), expected.end());
    auto actual = original;
    const auto* address = actual.data();
    const auto capacity = actual.capacity();
    workspace.sort(actual);
    SIRKIT_TEST_REQUIRE(actual == expected);
    SIRKIT_TEST_REQUIRE(actual.data() == address && actual.capacity() == capacity);
}

template<class T> static void sort_failures() {
    const auto original = make_values<T>(1025);
    auto expected = original;
    std::stable_sort(expected.begin(), expected.end());
    for (bool use_workspace : {false, true}) {
        bool succeeded = false, saw_failure = false;
        for (int budget = 0; budget < 32; ++budget) {
            auto values = original;
            const auto* address = values.data();
            const auto capacity = values.capacity();
            RadixSortWorkspace<T> workspace;
            const auto retained = live;
            const bool failed = fails_with_budget(budget, [&] {
                if (use_workspace) workspace.sort(values);
                else RadixSort(values);
            });
            SIRKIT_TEST_REQUIRE(values.data() == address && values.capacity() == capacity);
            if (failed) {
                saw_failure = true;
                SIRKIT_TEST_REQUIRE(values == original);
                SIRKIT_TEST_REQUIRE(workspace.capacity() == 0);
                SIRKIT_TEST_REQUIRE(live == retained);
                inspect_values(original, workspace);
            } else {
                SIRKIT_TEST_REQUIRE(values == expected);
                succeeded = true;
                break;
            }
        }
        SIRKIT_TEST_REQUIRE(succeeded && saw_failure);
    }
    RadixSortWorkspace<T> reserved;
    reserved.reserve(original.size());
    const auto capacity = reserved.capacity();
    auto values = original;
    const auto retained = live;
    SIRKIT_TEST_REQUIRE(!fails_with_budget(0, [&] { reserved.sort(values); }));
    SIRKIT_TEST_REQUIRE(values == expected && reserved.capacity() == capacity);
    SIRKIT_TEST_REQUIRE(live == retained);
    SIRKIT_TEST_REQUIRE(fails_with_budget(0, [&] { reserved.reserve(capacity + 1); }));
    SIRKIT_TEST_REQUIRE(reserved.capacity() == capacity && live == retained);
    bool too_large = false;
    try { reserved.reserve(std::numeric_limits<std::size_t>::max()); }
    catch (const std::length_error&) { too_large = true; }
    SIRKIT_TEST_REQUIRE(too_large && reserved.capacity() == capacity && live == retained);
    reserved.release();
    SIRKIT_TEST_REQUIRE(reserved.capacity() == 0);
    reserved.release();
    SIRKIT_TEST_REQUIRE(reserved.capacity() == 0);
    inspect_values(original, reserved);
}

template<class T> static void order_failures() {
    const auto values = make_values<T>(257);
    const auto saved = values;
    std::vector<std::size_t> expected(values.size());
    std::iota(expected.begin(), expected.end(), std::size_t{0});
    std::stable_sort(expected.begin(), expected.end(), [&](auto a, auto b) {
        return values[a] < values[b];
    });
    const auto retained = live;
    bool succeeded = false, saw_failure = false;
    for (int budget = 0; budget < 32; ++budget) {
        std::vector<std::size_t> answer;
        const bool failed = fails_with_budget(budget, [&] { answer = RadixSortOrder(values); });
        SIRKIT_TEST_REQUIRE(values == saved);
        if (failed) { saw_failure = true; SIRKIT_TEST_REQUIRE(live == retained); }
        else { SIRKIT_TEST_REQUIRE(answer == expected); succeeded = true; break; }
    }
    SIRKIT_TEST_REQUIRE(succeeded && saw_failure);
    SIRKIT_TEST_REQUIRE(live == retained);
}

template<class T> static void workspace_ownership() {
    const auto values = make_values<T>(1025);
    RadixSortWorkspace<T> source;
    inspect_values(values, source);
    const auto source_capacity = source.capacity();
    for (bool assignment : {false, true}) {
        bool succeeded = false, saw_failure = false;
        for (int budget = 0; budget < 32; ++budget) {
            RadixSortWorkspace<T> target;
            target.reserve(16);
            const auto capacity = target.capacity();
            const auto retained = live;
            const bool failed = fails_with_budget(budget, [&] {
                if (assignment) target = source;
                else { RadixSortWorkspace<T> copy(source); target = std::move(copy); }
            });
            SIRKIT_TEST_REQUIRE(source.capacity() == source_capacity);
            if (failed) {
                saw_failure = true;
                SIRKIT_TEST_REQUIRE(target.capacity() == capacity && live == retained);
                inspect_values(values, target);
            } else {
                target.release();
                SIRKIT_TEST_REQUIRE(target.capacity() == 0 && source.capacity() == source_capacity);
                inspect_values(values, source);
                succeeded = true;
                break;
            }
        }
        SIRKIT_TEST_REQUIRE(succeeded && saw_failure);
    }
    RadixSortWorkspace<T> copied = source;
    copied.release();
    SIRKIT_TEST_REQUIRE(source.capacity() == source_capacity);
    RadixSortWorkspace<T> moved(std::move(source));
    source.release();
    inspect_values(values, moved);
    inspect_values(values, source);
    copied = std::move(moved);
    moved.release();
    inspect_values(values, copied);
    inspect_values(values, moved);
}

template<class T> static void allocation_free_paths() {
    for (int kind = 0; kind < 4; ++kind) {
        auto values = kind == 0 ? make_values<T>(64) : make_values<T>(257);
        if (kind == 1) std::sort(values.begin(), values.end());
        if (kind == 2) std::sort(values.rbegin(), values.rend());
        if (kind == 3) std::fill(values.begin(), values.end(), T{7});
        auto expected = values;
        std::stable_sort(expected.begin(), expected.end());
        const auto retained = live;
        RadixSortWorkspace<T> workspace;
        SIRKIT_TEST_REQUIRE(!fails_with_budget(0, [&] { workspace.sort(values); }));
        SIRKIT_TEST_REQUIRE(values == expected && workspace.capacity() == 0 && live == retained);
    }
    std::vector<T> empty;
    SIRKIT_TEST_REQUIRE(!fails_with_budget(0, [&] {
        RadixSortWorkspace<T> workspace;
        workspace.sort(empty);
        SIRKIT_TEST_REQUIRE(RadixSortOrder(empty).empty());
    }));
}

template<class T> static void invalid_maximum() {
#ifndef SKIP_RUNTIME_CHECKS
    const T maximum = std::is_signed_v<T> ? T(-1) : T(1);
    const T valid = std::is_signed_v<T> ? T(-7) : T(0);
    const T invalid = T(maximum + 1);
    RadixSortWorkspace<T> workspace;
    workspace.reserve(1025);
    const auto capacity = workspace.capacity();
    for (int n : {1, 16, 64, 65, 127, 128, 129, 257, 1025}) {
        for (int position : {0, n / 2, n - 1}) {
            std::vector<T> values(n, valid);
            if (n > 3) values[0] = maximum;
            values[static_cast<std::size_t>(position)] = invalid;
            const auto saved = values;
            const auto* address = values.data();
            const auto value_capacity = values.capacity();
            const auto retained = live;
            bool rejected = false;
            try { workspace.sort(values, maximum); }
            catch (const std::out_of_range&) { rejected = true; }
            SIRKIT_TEST_REQUIRE(rejected && workspace.capacity() == capacity && live == retained);
            SIRKIT_TEST_REQUIRE(values.data() == address && values.capacity() == value_capacity);
            // 小配列は例外までに整列されてもよい。値の消失は認めない。
            auto sorted_saved = saved;
            std::sort(sorted_saved.begin(), sorted_saved.end());
            std::sort(values.begin(), values.end());
            SIRKIT_TEST_REQUIRE(values == sorted_saved);
        }
        std::vector<T> values(n, valid);
        workspace.sort(values, maximum);
        SIRKIT_TEST_REQUIRE(workspace.capacity() == capacity && std::is_sorted(values.begin(), values.end()));
    }
#endif
}

template<class T> static void test_type() {
    sort_failures<T>(); order_failures<T>(); workspace_ownership<T>();
    allocation_free_paths<T>(); invalid_maximum<T>();
}
int main() {
    const auto beginning = live;
    test_type<std::int32_t>(); test_type<std::uint32_t>();
    test_type<std::int64_t>(); test_type<std::uint64_t>();
    SIRKIT_TEST_REQUIRE(live == beginning);
    std::printf("PASS radix ownership checks=%ju allocation_failures=%ju live_delta=%jd\n",
                static_cast<std::uintmax_t>(checks), static_cast<std::uintmax_t>(allocation_failures),
                static_cast<std::intmax_t>(live) - static_cast<std::intmax_t>(beginning));
}
