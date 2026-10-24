// SPDX-License-Identifier: CC0-1.0
// 独立した成分ラベルとの照合、全int域、所有権、確保失敗を検査する。
#ifndef DUF_SOURCE
#define DUF_SOURCE "../../../DataStructure/dynamic-union-find.cpp"
#endif
#include DUF_SOURCE
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <new>
#include <random>
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

// Union-Findを使わず、成分ラベルを全要素の走査で置き換える。
struct Reference {
    std::map<int, int> labels;
    bool add(int key) { return labels.emplace(key, key).second; }
    bool same(int a, int b) {
        add(a);
        add(b);
        return labels.at(a) == labels.at(b);
    }
    bool unite(int a, int b) {
        if (same(a, b)) return false;
        const int from = labels.at(b), to = labels.at(a);
        for (auto& [key, label] : labels)
            if (label == from) label = to;
        return true;
    }
    int size(int key) {
        add(key);
        const int target = labels.at(key);
        int result = 0;
        for (const auto& [other, label] : labels) result += label == target;
        return result;
    }
    std::vector<std::vector<int>> groups() const {
        std::map<int, std::vector<int>> grouped;
        for (auto [key, label] : labels) grouped[label].push_back(key);
        std::vector<std::vector<int>> result;
        for (auto& [label, group] : grouped) result.push_back(std::move(group));
        std::sort(result.begin(), result.end());
        return result;
    }
};

static std::vector<std::vector<int>> normalized(DynamicUnionFind& tree) {
    auto groups = tree.groups();
    for (auto& group : groups) std::sort(group.begin(), group.end());
    std::sort(groups.begin(), groups.end());
    return groups;
}

static void validate(DynamicUnionFind& tree, Reference& ref) {
    const auto expected = ref.groups();
    require(tree.node_count() == ref.labels.size());
    require(tree.component_count() == expected.size());
    require(tree.empty() == ref.labels.empty());
    require(normalized(tree) == expected);
    for (auto [key, label] : ref.labels) {
        require(tree.contains(key));
        const int root = tree.find(key);
        require(ref.labels.contains(root) && ref.labels.at(root) == label);
        require(tree.find(root) == root);
        require(tree.size(key) == ref.size(key));
        const int data = tree.data(key);
        if (root == key) require(data == -ref.size(key));
        else {
            // 負の親キーも許されるので、符号で根とは判定しない。
            require(ref.labels.contains(data) && ref.labels.at(data) == label);
            require(tree.find(data) == root);
        }
    }
}

[[gnu::noinline]] static void observe(const DynamicUnionFind& tree) {
    volatile std::size_t count = tree.node_count();
    volatile bool contains = tree.contains(std::numeric_limits<int>::min());
    (void)count;
    (void)contains;
}

static void boundary_and_ownership() {
    DynamicUnionFind tree;
    Reference ref;
    require(tree.empty() && tree.node_count() == 0 && tree.component_count() == 0);
    require(!tree.contains(123));
    require(tree.empty());
    require(tree.data(-1) == -1);
    ref.add(-1);
    require(tree.find(std::numeric_limits<int>::min()) == std::numeric_limits<int>::min());
    ref.add(std::numeric_limits<int>::min());
    require(tree.size(std::numeric_limits<int>::max()) == 1);
    ref.add(std::numeric_limits<int>::max());
    require(tree.same(-2, -2));
    ref.add(-2);
    require(!tree.same(-3, 0));
    ref.add(-3); ref.add(0);
    require(tree.unite(-1, std::numeric_limits<int>::min()) == ref.unite(-1, std::numeric_limits<int>::min()));
    require(tree.unite(-2, -3) == ref.unite(-2, -3));
    require(tree.unite(-2, 0) == ref.unite(-2, 0));
    validate(tree, ref);

    int callbacks = 0;
    const int left = tree.find(-1), right = tree.find(-2);
    const int combined = tree.size(-1) + tree.size(-2);
    require(ref.unite(-1, -2));
    require(tree.unite(-1, -2, [&](int root, int absorbed) {
        ++callbacks;
        require((root == left && absorbed == right) || (root == right && absorbed == left));
        require(tree.find(root) == root && tree.find(absorbed) == root);
        require(tree.size(root) == combined);
    }));
    require(callbacks == 1);
    require(!tree.unite(-1, 0, [&](int, int) { ++callbacks; }));
    require(callbacks == 1);

    struct CallbackException {};
    ref.add(100); ref.add(200); ref.unite(100, 200);
    bool caught = false;
    try {
        tree.unite(100, 200, [&](int root, int absorbed) {
            require(tree.find(absorbed) == root && tree.size(root) == 2);
            throw CallbackException{};
        });
    } catch (CallbackException) { caught = true; }
    require(caught && tree.same(100, 200));
    validate(tree, ref);

    auto copy = tree;
    validate(copy, ref);
    tree = tree;
    validate(tree, ref);
    copy.add(33);
    require(!tree.contains(33));
    copy = tree;
    require(!copy.contains(33));
    DynamicUnionFind moved(std::move(copy));
    require(copy.empty() && copy.component_count() == 0);
    require(!copy.contains(-1));
    copy.add(77);
    require(copy.find(77) == 77 && !moved.contains(77));
    validate(moved, ref);
    DynamicUnionFind assigned;
    assigned.add(55);
    assigned = std::move(moved);
    require(moved.empty() && moved.component_count() == 0 && !assigned.contains(55));
    moved.unite(1, 2);
    require(moved.size(2) == 2 && !assigned.contains(1));
    validate(assigned, ref);
    assigned = std::move(assigned);
    validate(assigned, ref);

    const auto before = normalized(tree);
    bool length = false;
    try { tree.reserve(std::size_t(std::numeric_limits<int>::max()) + 1); }
    catch (const std::length_error&) { length = true; }
    require(length && normalized(tree) == before);
    length = false;
    try { DynamicUnionFind invalid(std::numeric_limits<std::size_t>::max()); observe(invalid); }
    catch (const std::length_error&) { length = true; }
    require(length);

    DynamicUnionFind reserved(256);
    Reference known;
    for (int i = -32; i < 32; ++i) { reserved.add(i); known.add(i); }
    const auto before_allocations = allocations;
    allocation_budget = 0;
    reserved.reserve(128);
    for (int i = -32; i < 31; ++i) {
        require(reserved.contains(i));
        require(reserved.find(i) == i);
        require(reserved.size(i) == 1);
        require(reserved.data(i) == -1);
        require(reserved.same(i, i));
        require(!reserved.add(i));
    }
    allocation_budget = -1;
    require(allocations == before_allocations);
    allocation_budget = 0;
    reserved.clear();
    require(reserved.empty());
    for (int i = -32; i < 32; ++i) require(reserved.add(i));
    allocation_budget = -1;
    require(allocations == before_allocations);
    validate(reserved, known);
}

static void randomized() {
    for (unsigned seed = 0; seed != 6; ++seed) {
        std::mt19937 rng(0x4289U + seed);
        std::uniform_int_distribution<int> integer(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        std::vector<int> keys{std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), -1, 0, 1, -2, 2, -1'000'000'000, 1'000'000'000};
        while (keys.size() < 64) keys.push_back(integer(rng));
        DynamicUnionFind tree;
        Reference ref;
        for (unsigned step = 0; step != 1200; ++step) {
            const int x = keys[rng() % keys.size()], y = keys[rng() % keys.size()];
            if (step && step % 173 == 0) { tree.clear(); ref.labels.clear(); }
            switch (rng() % 12) {
                case 0: require(tree.add(x) == ref.add(x)); break;
                case 1: {
                    ref.add(x);
                    const int root = tree.find(x);
                    require(ref.labels.contains(root) && ref.labels.at(root) == ref.labels.at(x));
                    break;
                }
                case 2: require(tree.size(x) == ref.size(x)); break;
                case 3: require(bool(tree.same(x, y)) == ref.same(x, y)); break;
                case 4: require(bool(tree.unite(x, y)) == ref.unite(x, y)); break;
                case 5: {
                    ref.add(x); ref.add(y);
                    const int a = tree.find(x), b = tree.find(y);
                    const bool merged = ref.unite(x, y);
                    int calls = 0;
                    require(bool(tree.unite(x, y, [&](int root, int absorbed) {
                        ++calls;
                        require(root != absorbed && ((root == a && absorbed == b) || (root == b && absorbed == a)));
                        require(tree.find(root) == root && tree.find(absorbed) == root);
                        require(tree.size(root) == ref.size(x));
                    })) == merged);
                    require(calls == int(merged));
                    break;
                }
                case 6: {
                    ref.add(x);
                    const int data = tree.data(x), root = tree.find(x);
                    if (root == x) require(data == -ref.size(x));
                    else require(ref.labels.contains(data) && ref.labels.at(data) == ref.labels.at(x));
                    break;
                }
                case 7: require(tree.contains(x) == ref.labels.contains(x)); break;
                case 8: tree.reserve(keys.size() + 1); break;
                case 9: {
                    DynamicUnionFind copy(tree);
                    validate(copy, ref);
                    copy = tree;
                    validate(copy, ref);
                    copy.clear();
                    require(copy.empty());
                    break;
                }
                case 10: {
                    DynamicUnionFind copy(tree);
                    DynamicUnionFind moved(std::move(copy));
                    require(copy.empty() && copy.component_count() == 0);
                    validate(moved, ref);
                    copy = std::move(moved);
                    require(moved.empty() && moved.component_count() == 0);
                    validate(copy, ref);
                    break;
                }
                case 11: require(normalized(tree) == ref.groups()); break;
            }
            require(tree.node_count() == ref.labels.size());
            require(tree.component_count() == ref.groups().size());
            if (step % 23 == 0) validate(tree, ref);
        }
        validate(tree, ref);
    }
}

static DynamicUnionFind prepared(int count) {
    DynamicUnionFind tree;
    for (int i = 0; i < count; ++i) tree.add(i * 7919 - 123456);
    for (int i = 1; i < count; i += 3) tree.unite(i * 7919 - 123456, (i - 1) * 7919 - 123456);
    return tree;
}

static void allocation_failures() {
    for (int count : {0, 6, 8, 12, 16, 24, 32, 48}) {
        bool passed = false;
        for (int budget = 0; budget != 8; ++budget) {
            auto tree = prepared(count);
            const auto before = normalized(tree);
            const auto nodes = tree.node_count(), components = tree.component_count();
            allocation_budget = budget;
            bool failed = false;
            try { tree.add(std::numeric_limits<int>::min()); }
            catch (const std::bad_alloc&) { failed = true; }
            allocation_budget = -1;
            if (failed) {
                ++failure_points;
                require(tree.node_count() == nodes && tree.component_count() == components);
                require(!tree.contains(std::numeric_limits<int>::min()));
                require(normalized(tree) == before);
                // 失敗後も同じキーを追加し直せる。
                require(tree.add(std::numeric_limits<int>::min()));
                require(tree.size(std::numeric_limits<int>::min()) == 1);
            } else {
                require(tree.node_count() == nodes + 1 && tree.component_count() == components + 1);
                require(tree.contains(std::numeric_limits<int>::min()));
                passed = true;
                break;
            }
        }
        require(passed);
    }
    const auto source = prepared(63);
    for (int operation = 0; operation != 3; ++operation) {
        bool passed = false;
        for (int budget = 0; budget != 10; ++budget) {
            auto target = prepared(7);
            const auto before = normalized(target);
            allocation_budget = budget;
            bool failed = false;
            try {
                if (operation == 0) target = source;
                else if (operation == 1) { DynamicUnionFind copy(source); observe(copy); }
                else target.reserve(512);
            } catch (const std::bad_alloc&) { failed = true; }
            allocation_budget = -1;
            if (failed) {
                ++failure_points;
                require(target.node_count() == 7 && normalized(target) == before);
                require(target.contains(-123456) && !target.contains(std::numeric_limits<int>::min()));
                target.add(std::numeric_limits<int>::min());
                require(target.size(std::numeric_limits<int>::min()) == 1);
            } else {
                passed = true;
                if (operation == 0) require(target.node_count() == source.node_count());
                else require(target.node_count() == 7 && normalized(target) == before);
                break;
            }
        }
        require(passed);
    }
}

static void groups_boundaries() {
    constexpr int sizes[] = {0, 1, 31, 32, 33, 64};
    for (int n : sizes) {
        DynamicUnionFind tree;
        Reference ref;
        for (int i = 0; i < n; ++i) {
            const int key = 1009 * i - 19897;
            tree.add(key);
            ref.add(key);
        }
        validate(tree, ref);
        for (int i = 1; i < n; i += 2) {
            tree.unite(1009 * (i - 1) - 19897, 1009 * i - 19897);
            ref.unite(1009 * (i - 1) - 19897, 1009 * i - 19897);
        }
        validate(tree, ref);
        for (int i = 1; i < n; ++i) {
            tree.unite(-19897, 1009 * i - 19897);
            ref.unite(-19897, 1009 * i - 19897);
        }
        validate(tree, ref);
        tree.clear();
        ref.labels.clear();
        validate(tree, ref);
    }
}

int main() {
    const auto initial_live = live_allocations;
    boundary_and_ownership();
    groups_boundaries();
    randomized();
    allocation_failures();
    require(live_allocations == initial_live);
    std::printf("{\"status\":\"PASS\",\"checks\":%zu,\"allocation_failure_points\":%zu,\"live_delta\":%zd}\n",
                checks, failure_points, static_cast<std::ptrdiff_t>(live_allocations) - static_cast<std::ptrdiff_t>(initial_live));
}
