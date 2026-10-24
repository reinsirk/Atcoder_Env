// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_RUF_SOURCE
#define SIRKIT_RUF_SOURCE "../../../DataStructure/rollback-union-find.cpp"
#endif
#ifndef SIRKIT_RWUF_SOURCE
#define SIRKIT_RWUF_SOURCE "../../../DataStructure/rollback-union-findw.cpp"
#endif
#include SIRKIT_RUF_SOURCE
#include SIRKIT_RWUF_SOURCE
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
static void prepare(RollbackUnionFind& uf) {
    const int middle = (uf.size() / 2) & ~1;
    for (int i = 0; i + 1 < middle; i += 2) require(uf.unite(i, i + 1));
    uf.snapshot();
    for (int i = middle; i + 1 < uf.size(); i += 2) require(uf.unite(i, i + 1));
    for (int i = 0; i < 10; ++i) require(!uf.unite(0, 0));
}
static void equal(const RollbackUnionFind& a, const RollbackUnionFind& b) {
    require(a.size() == b.size() && a.component_count() == b.component_count() && a.get_state() == b.get_state());
    for (int i = 0; i < a.size(); ++i) require(a.find(i) == b.find(i) && a.size(i) == b.size(i));
}
static void equal_history(const RollbackUnionFind& a, const RollbackUnionFind& b) {
    equal(a, b); auto left = a, right = b; left.rollback(); right.rollback(); equal(left, right);
}
static void plain_reuse() {
    RollbackUnionFind source(65), target(128); prepare(source); prepare(target); target.reserve_history(1000);
    auto initial = source; initial.rollback();
    const auto before = allocations; allocation_budget = 0;
    target = source; equal(target, source); target.rollback(); equal(target, initial);
    target = source; auto* alias = &target; target = *alias; equal(target, source);
    target.clear_history(); require(target.get_state() == 0 && target.component_count() == source.component_count());
    target.reset(); require(target.size() == 65 && target.component_count() == 65 && target.get_state() == 0);
    for (int i = 0; i < target.size(); ++i) require(target.find(i) == i && target.size(i) == 1);
    target.unite(0, 1); target.snapshot(); target.clear(); require(target.empty() && target.size() == 0 && target.get_state() == 0);
    for (const std::size_t n : {0, 1, 17, 128, 5}) { target.reset(n); require(target.size() == static_cast<int>(n) && target.component_count() == static_cast<int>(n)); }
    swap(target, source); swap(target, source);
    allocation_budget = -1; require(allocations == before);
    static_assert(noexcept(target.clear()) && noexcept(target.empty()) && noexcept(swap(target, source)));
}
static void plain_failures() {
    for (int operation = 0; operation < 2; ++operation) {
        bool success = false;
        for (int budget = 0; budget < 8; ++budget) {
            const auto before = live;
            {
                RollbackUnionFind source(129), target(17); prepare(source); prepare(target); const auto old = target;
                allocation_budget = budget; bool caught = false;
                try { if (operation == 0) target = source; else target.reset(512); }
                catch (const std::bad_alloc&) { caught = true; ++failures; }
                allocation_budget = -1;
                if (caught) equal_history(target, old);
                else { if (operation == 0) equal_history(target, source); else require(target.size() == 512 && target.component_count() == 512 && target.get_state() == 0); success = true; }
            }
            require(live == before); if (success) break;
        }
        require(success);
    }
    for (int budget : {0, 1}) {
        const auto before = live;
        {
            RollbackUnionFind source(65), target; prepare(source);
            allocation_budget = budget; bool caught = false;
            try { target = source; } catch (const std::bad_alloc&) { caught = true; ++failures; }
            allocation_budget = -1; require(caught && target.empty() && target.get_state() == 0); target.rollback();
            target = source; equal_history(target, source);
        }
        require(live == before);
    }
}
static void groups_preserve_rollback() {
    for (int n : {1, 2, 17, 128}) for (int root : {0, n / 2, n - 1}) {
        RollbackUnionFind uf(n);
        for (int i = 0; i < n; ++i) if (i != root) require(uf.unite(root, i));
        const int state = uf.get_state(); const auto groups = uf.groups();
        require(groups.size() == 1 && groups[0].size() == static_cast<std::size_t>(n) && uf.get_state() == state);
        for (int i = 0; i < n; ++i) require(groups[0][i] == i && uf.find(i) == root && uf.size(i) == n);
        uf.rollback(0); require(uf.component_count() == n);
        for (int i = 0; i < n; ++i) require(uf.find(i) == i && uf.size(i) == 1);
    }
}
struct ValueFailure {};
struct Value {
    static inline int alive = 0;
    static inline bool fail_zero = false, fail_move = false, fail_assignment = false;
    std::int64_t value;
    explicit Value(std::int64_t x = 0) : value(x) { if (x == 0 && fail_zero) throw ValueFailure{}; ++alive; }
    Value(const Value& other) noexcept : value(other.value) { ++alive; }
    Value(Value&& other) noexcept(false) : value(other.value) { if (fail_move) throw ValueFailure{}; ++alive; }
    Value& operator=(const Value& other) { value = other.value; if (fail_assignment) { value = -999; throw ValueFailure{}; } return *this; }
    Value& operator=(Value&& other) { if (fail_move) throw ValueFailure{}; value = other.value; if (fail_assignment) { value = -999; throw ValueFailure{}; } return *this; }
    ~Value() { --alive; }
    friend Value operator+(const Value& a, const Value& b) { return Value(a.value + b.value); }
    friend Value operator-(const Value& a, const Value& b) { return Value(a.value - b.value); }
    friend bool operator==(const Value& a, const Value& b) { return a.value == b.value; }
};
static_assert(std::is_nothrow_copy_constructible_v<Value> && !std::is_nothrow_move_constructible_v<Value>);
template<class T> static std::int64_t number(const T& value) { if constexpr (std::is_same_v<T, Value>) return value.value; else return static_cast<std::int64_t>(value); }
template<class T> static void disconnected(const RollbackWeightedUnionFind<T>& uf, int n) {
    require(uf.size() == n && uf.get_state() == 0 && uf.component_count() == n);
    for (int i = 0; i < n; ++i) require(uf.find(i) == i && uf.size(i) == 1 && number(uf.get_weight(i)) == 0);
}
template<class T> static void weighted_reuse() {
    RollbackWeightedUnionFind<T> uf(128); uf.reserve_history(1000);
    for (int i = 1; i < 128; ++i) require(uf.unite(0, i, T(i)));
    uf.snapshot(); const auto before = allocations; allocation_budget = 0;
    uf.reset(); disconnected(uf, 128);
    for (int n : {0, 1, 17, 128, 5}) { uf.reset(n); disconnected(uf, n); }
    uf.unite(0, 1, T(7)); uf.rollback(); disconnected(uf, 5);
    allocation_budget = -1; require(allocations == before);
}
static void weighted_failures() {
    for (int budget = 0; budget < 3; ++budget) {
        const auto before = live;
        {
            RollbackWeightedUnionFind<Value> uf;
            allocation_budget = budget; bool caught = false;
            try { uf.reset(129); } catch (const std::bad_alloc&) { caught = true; ++failures; }
            allocation_budget = -1; require(caught); disconnected(uf, 0);
            uf.reset(129); disconnected(uf, 129);
        }
        require(live == before && Value::alive == 0);
    }
    {
        RollbackWeightedUnionFind<Value> uf(4); uf.unite(0, 1, Value(3)); uf.snapshot();
        Value::fail_zero = true; bool caught = false;
        try { uf.reset(); } catch (const ValueFailure&) { caught = true; }
        Value::fail_zero = false;
        require(caught && uf.get_state() == 1 && uf.component_count() == 3 && uf.diff(0, 1).value == 3);
        Value::fail_move = true; uf.reset(128); Value::fail_move = false; disconnected(uf, 128);
    }
    require(Value::alive == 0);
}
static void destructive_assignment_regression() {
    RollbackWeightedUnionFind<Value> uf(4); uf.unite(0, 1, Value(3)); uf.unite(2, 3, Value(7));
    Value::fail_assignment = true; Value::fail_zero = true; uf.undo(); Value::fail_zero = false; Value::fail_assignment = false;
    require(uf.get_state() == 1 && uf.component_count() == 3 && uf.diff(0, 1).value == 3 && uf.get_weight(3).value == 0);
    Value::fail_assignment = true; bool caught = false;
    try { uf.unite(0, 2, Value(9)); } catch (const ValueFailure&) { caught = true; }
    Value::fail_assignment = false;
    require(caught && uf.get_state() == 1 && !uf.same(0, 2) && uf.get_weight(2).value == 0 && uf.diff(0, 1).value == 3);
    uf.reset(); disconnected(uf, 4);
}
static void bounds_and_empty_zero() {
    RollbackUnionFind uf(4); uf.unite(0, 1); uf.snapshot();
    bool caught = false;
    try { uf.reset(static_cast<std::size_t>(std::numeric_limits<int>::max()) + 1); }
    catch (const std::length_error&) { caught = true; }
    require(caught && uf.size() == 4 && uf.get_state() == 1 && uf.same(0, 1));
    uf.rollback(); require(uf.get_state() == 1);
    RollbackWeightedUnionFind<std::int64_t> weighted(4); weighted.unite(0, 1, 7); weighted.snapshot();
    caught = false;
    try { weighted.reset(-1); }
#ifndef SKIP_RUNTIME_CHECKS
    catch (const std::invalid_argument&) { caught = true; }
#else
    catch (const std::length_error&) { caught = true; }
#endif
    require(caught && weighted.size() == 4 && weighted.get_state() == 1 && weighted.diff(0, 1) == 7);
    weighted.rollback(); require(weighted.get_state() == 1);
    RollbackWeightedUnionFind<Value> empty;
    Value::fail_zero = true; caught = false;
    try { empty.reset(0); } catch (const ValueFailure&) { caught = true; }
    Value::fail_zero = false; require(caught && empty.empty() && empty.get_state() == 0);
    RollbackWeightedUnionFind<Value> reused(8); reused.unite(0, 1, Value(9));
    Value::fail_assignment = true; Value::fail_move = true;
    reused.reset();
    Value::fail_assignment = false; Value::fail_move = false;
    disconnected(reused, 8);
}
static void signed_reset() {
    RollbackWeightedUnionFind<std::int64_t> uf(4);
    require(uf.unite(0, 1, std::numeric_limits<std::int64_t>::max())); uf.snapshot();
    uf.reset(); require(uf.unite(1, 2, 7)); uf.rollback(); disconnected(uf, 4);
    require(uf.unite(0, 1, std::numeric_limits<std::int64_t>::min())); require(uf.diff(0, 1) == std::numeric_limits<std::int64_t>::min());
}
int main() {
    const auto before = live;
    plain_reuse(); plain_failures(); groups_preserve_rollback();
    weighted_reuse<std::int64_t>(); weighted_reuse<Value>(); weighted_failures(); destructive_assignment_regression(); bounds_and_empty_zero(); signed_reset();
    require(live == before && Value::alive == 0);
    std::printf("PASS rollback reuse checks=%zu injected_failures=%zu live_delta=%td\n", checks, failures, static_cast<std::ptrdiff_t>(live) - static_cast<std::ptrdiff_t>(before));
}
