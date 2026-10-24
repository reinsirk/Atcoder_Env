// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_UF_SOURCE
#define SIRKIT_UF_SOURCE "../../../DataStructure/union_find.cpp"
#endif
#ifndef SIRKIT_WUF_SOURCE
#define SIRKIT_WUF_SOURCE "../../../DataStructure/union_find_w.cpp"
#endif
#include SIRKIT_UF_SOURCE
#include SIRKIT_WUF_SOURCE
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <type_traits>

static std::ptrdiff_t allocation_budget = -1;
static std::size_t live = 0, allocations = 0, checks = 0, failures = 0;
[[gnu::noinline]] void* operator new(std::size_t count) {
    if (allocation_budget == 0) throw std::bad_alloc();
    if (allocation_budget > 0) --allocation_budget;
    if (void* p = std::malloc(count ? count : 1)) { ++live; ++allocations; return p; }
    throw std::bad_alloc();
}
[[gnu::noinline]] void* operator new[](std::size_t count) { return ::operator new(count); }
[[gnu::noinline]] static void release(void* p) noexcept { if (p) --live; std::free(p); }
void operator delete(void* p) noexcept { release(p); }
void operator delete[](void* p) noexcept { release(p); }
void operator delete(void* p, std::size_t) noexcept { release(p); }
void operator delete[](void* p, std::size_t) noexcept { release(p); }
static void require(bool value) { ++checks; if (!value) std::abort(); }
struct ValueFailure {};
struct Value {
    static inline int alive = 0;
    static inline bool fail_default = false, fail_move = false;
    std::int64_t value;
    Value() : value(0) { if (fail_default) throw ValueFailure{}; ++alive; }
    explicit Value(std::int64_t x) noexcept : value(x) { ++alive; }
    Value(const Value& other) noexcept : value(other.value) { ++alive; }
    Value(Value&& other) noexcept(false) : value(other.value) { if (fail_move) throw ValueFailure{}; ++alive; }
    Value& operator=(const Value& other) noexcept { value = other.value; return *this; }
    Value& operator=(Value&& other) noexcept(false) { if (fail_move) throw ValueFailure{}; value = other.value; return *this; }
    ~Value() { --alive; }
    Value& operator+=(const Value& other) noexcept { value += other.value; return *this; }
    friend Value operator+(const Value& a, const Value& b) noexcept { return Value(a.value + b.value); }
    friend Value operator-(const Value& a, const Value& b) noexcept { return Value(a.value - b.value); }
    friend Value operator-(const Value& a) noexcept { return Value(-a.value); }
    friend bool operator==(const Value& a, const Value& b) noexcept { return a.value == b.value; }
};
static_assert(std::is_nothrow_copy_constructible_v<Value>);
static_assert(!std::is_nothrow_move_constructible_v<Value>);
static_assert(!std::is_nothrow_default_constructible_v<Value>);
template<class T> static std::int64_t number(const T& value) {
    if constexpr (std::is_same_v<T, Value>) return value.value;
    else return static_cast<std::int64_t>(value);
}
template<class T> static void prepare(UnionFindW<T>& uf) {
    for (int i = 0; i + 1 < uf.size(); i += 2) require(uf.merge(i, i + 1, T(7)));
}
template<class T> static void inspect(UnionFindW<T>& uf, int n, bool connected) {
    require(uf.size() == n && uf.component_count() == (connected ? (n + 1) / 2 : n));
    for (int i = 0; i < n; ++i) {
        require(uf.root(i) == (connected ? i - i % 2 : i));
        require(uf.size(i) == (connected && (i % 2 || i + 1 < n) ? 2 : 1));
        require(number(uf.weight(i)) == (connected && i % 2 ? 7 : 0));
    }
}
template<class T> static void reuse_without_allocations() {
    const auto before = live;
    {
        UnionFindW<T> source(65), target(128); prepare(source); prepare(target);
        const auto calls_before = allocations; allocation_budget = 0;
        target.reset(); inspect(target, 128, false);
        target = source; inspect(target, 65, true); inspect(source, 65, true);
        target.clear(); require(target.empty());
        target = source; inspect(target, 65, true);
        auto* alias = &target; target = *alias; inspect(target, 65, true);
        for (const std::size_t n : {0, 1, 17, 128, 5}) { target.reset(n); inspect(target, static_cast<int>(n), false); }
        allocation_budget = -1; require(allocations == calls_before);
    }
    require(live == before);
}
static void allocation_failures() {
    for (int operation = 0; operation < 2; ++operation) {
        bool success = false;
        for (int budget = 0; budget < 8; ++budget) {
            const auto before = live;
            {
                UnionFindW<std::int64_t> source(512), target(65); prepare(source); prepare(target);
                bool failed = false; allocation_budget = budget;
                try { if (operation == 0) target.reset(512); else target = source; }
                catch (const std::bad_alloc&) { failed = true; ++failures; }
                allocation_budget = -1;
                inspect(source, 512, true);
                if (failed) inspect(target, 65, true);
                else { inspect(target, 512, operation != 0); success = true; }
            }
            require(live == before); if (success) break;
        }
        require(success);
    }
    // 空の代入先も、途中確保に失敗した場合は論理状態を維持する。
    for (int budget : {0, 1}) {
        const auto before = live;
        {
            UnionFindW<std::int64_t> source(65), target; prepare(source);
            allocation_budget = budget; bool caught = false;
            try { target = source; } catch (const std::bad_alloc&) { caught = true; ++failures; }
            allocation_budget = -1; require(caught); inspect(target, 0, false); inspect(source, 65, true);
            target = source; inspect(target, 65, true);
        }
        require(live == before);
    }
}
static void value_edges() {
    const auto before = live;
    {
        UnionFindW<Value> source(65), target(17); prepare(source); prepare(target);
        Value::fail_default = true; bool caught = false;
        try { target.reset(); } catch (const ValueFailure&) { caught = true; }
        Value::fail_default = false; require(caught); inspect(target, 17, true);
        Value::fail_move = true;
        target = source; inspect(target, 65, true);
        target.reset(128); inspect(target, 128, false);
        Value::fail_move = false;
        Value::fail_default = true;
        target.reset(0); target.reset(); inspect(target, 0, false);
        UnionFindW<Value> empty; target = empty; inspect(target, 0, false);
        Value::fail_default = false;
    }
    require(live == before && Value::alive == 0);
}
static void plain_groups_and_reset() {
    for (const int n : {1, 2, 17, 128}) for (const int root : {0, n / 2, n - 1}) {
        UnionFind uf(n);
        for (int i = 0; i < n; ++i) uf.merge(root, i);
        const auto groups = uf.groups(); require(groups.size() == 1 && groups[0].size() == static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) require(groups[0][i] == i && uf.root(i) == root && uf.size(i) == n);
        const auto before = allocations; allocation_budget = 0; uf.reset(); allocation_budget = -1;
        require(allocations == before && uf.size() == n && uf.component_count() == n);
        for (int i = 0; i < n; ++i) require(uf.root(i) == i && uf.size(i) == 1);
    }
}
int main() {
    const auto before = live;
    reuse_without_allocations<std::int64_t>(); reuse_without_allocations<Value>();
    allocation_failures(); value_edges(); plain_groups_and_reset();
    require(live == before && Value::alive == 0);
    std::printf("PASS reuse checks=%zu injected_failures=%zu live_delta=%td\n", checks, failures, static_cast<std::ptrdiff_t>(live) - static_cast<std::ptrdiff_t>(before));
}
