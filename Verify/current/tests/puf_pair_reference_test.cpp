// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_PUF_SOURCE
#define SIRKIT_PUF_SOURCE "../../../DataStructure/persistent-union-find.cpp"
#endif
#include SIRKIT_PUF_SOURCE
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <numeric>
#include <random>
#include <string>

static std::size_t checks = 0;
static void require(bool good) { ++checks; if (!good) std::abort(); }
template<class E, class F> static void expect(F f) {
    bool caught = false; try { f(); } catch (const E&) { caught = true; } require(caught);
}
template<int Shift> static void random_array() {
    using Array = PersistentArray<int, Shift>;
    constexpr int width = 1 << Shift;
    const int n = width + 3;
    std::vector<int> initial(n); std::iota(initial.begin(), initial.end(), -20);
    Array array(initial, -7);
    std::vector<typename Array::Node*> versions{array.root};
    std::vector<std::vector<int>> models{initial};
    std::mt19937_64 random(2026102200 + Shift);
    for (int step = 0; step < 200; ++step) {
        const auto base = static_cast<std::size_t>(random() % versions.size());
        const int a = static_cast<int>(random() % n), b = static_cast<int>(random() % n);
        auto next = models[base]; next[a] = step; next[b] = -step;
        array.update_two(versions[base], a, step, b, -step);
        versions.push_back(array.root); models.push_back(std::move(next));
        for (int trial = 0; trial < 50; ++trial) {
            const auto version = static_cast<std::size_t>(random() % versions.size());
            const int x = static_cast<int>(random() % n), y = static_cast<int>(random() % n);
            const auto pair = array.get_two(versions[version], x, y);
            require(pair.first == models[version][x] && pair.second == models[version][y]);
            const auto current = array.get_two(x, y);
            require(current.first == models.back()[x] && current.second == models.back()[y]);
            const auto same = array.get_two(versions[version], x, x);
            require(same.first == models[version][x] && same.second == same.first);
        }
        require(array.get_two(nullptr, a, b) == std::pair{-7, -7});
    }
    Array sparse(n, -13);
    const auto sparse_initial = sparse.root;
    sparse.update(0, 42);
    require(sparse.get_two(0, width) == std::pair{42, -13});
    require(sparse.get_two(width, 0) == std::pair{-13, 42});
    require(sparse.get_two(sparse_initial, width, 0) == std::pair{-13, -13});
    for (const int size : {width - 1, width, width + 1, width * width - 1, width * width, width * width + 1}) {
        std::vector<int> values(static_cast<std::size_t>(size));
        std::iota(values.begin(), values.end(), 0);
        Array boundary(values, -9);
        for (int i = 0; i < size; ++i) {
            const auto pair = boundary.get_two(i, size - 1 - i);
            require(pair.first == i && pair.second == size - 1 - i);
        }
    }
#ifndef SKIP_RUNTIME_CHECKS
    expect<std::out_of_range>([&] { (void)array.get_two(-1, 0); });
    expect<std::out_of_range>([&] { (void)array.get_two(0, n); });
    Array foreign(n, -7);
    expect<std::invalid_argument>([&] { (void)array.get_two(foreign.root, 0, 0); });
    Array empty;
    expect<std::out_of_range>([&] { (void)empty.get_two(0, 0); });
#endif
    Array huge(std::numeric_limits<std::int64_t>::max(), -1);
    const auto last = huge.size() - 1;
    const auto old = huge.root;
    huge.update_two(0, 2, last, 3);
    require(huge.get_two(0, last) == std::pair{2, 3});
    require(huge.get_two(old, 0, last) == std::pair{-1, -1});
}
struct CopyOnly {
    int value;
    CopyOnly() = delete;
    explicit CopyOnly(int x) : value(x) {}
    CopyOnly(const CopyOnly&) = default;
    CopyOnly(CopyOnly&&) = delete;
    CopyOnly& operator=(const CopyOnly&) = default;
    CopyOnly& operator=(CopyOnly&&) = delete;
};
struct alignas(128) Aligned { int value; };
struct ValueFailure : std::exception {};
struct Throwing {
    static inline int live = 0, budget = -1;
    int value;
    explicit Throwing(int x = 0) : value(x) { ++live; }
    Throwing(const Throwing& other) : value(other.value) {
        if (budget == 0) throw ValueFailure{};
        if (budget > 0) --budget;
        ++live;
    }
    Throwing& operator=(const Throwing& other) { value = other.value; return *this; }
    ~Throwing() { --live; }
};
static void generic_values() {
    CopyOnly id(-1), x(7), y(8);
    PersistentArray<CopyOnly> a(33, id); const auto old = a.root;
    a.update_two(0, x, 32, y);
    const auto pair = a.get_two(0, 32);
    require(pair.first.value == 7 && pair.second.value == 8);
    const auto previous = a.get_two(old, 0, 32);
    require(previous.first.value == -1 && previous.second.value == -1);
    PersistentArray<Aligned> aligned(33, Aligned{-1});
    aligned.update_two(0, Aligned{9}, 32, Aligned{10});
    const auto aligned_pair = aligned.get_two(0, 32);
    require(aligned_pair.first.value == 9 && aligned_pair.second.value == 10);
    PersistentArray<std::string> text(33, std::string(100, 'a'));
    text.update_two(0, std::string(100, 'b'), 32, std::string(100, 'c'));
    require(text.get_two(0, 32) == std::pair(std::string(100, 'b'), std::string(100, 'c')));
    {
        PersistentArray<Throwing> values(33, Throwing(5));
        const auto root = values.root;
        const int live = Throwing::live;
        for (int budget : {0, 1}) {
            Throwing::budget = budget;
            expect<ValueFailure>([&] { (void)values.get_two(0, 32); });
            Throwing::budget = -1;
            require(Throwing::live == live && values.root == root);
            require(values.get(0).value == 5 && values.get(32).value == 5);
        }
    }
    require(Throwing::live == 0);
}
int main() {
    random_array<1>(); random_array<2>(); random_array<3>(); random_array<4>();
    random_array<5>(); random_array<6>(); random_array<7>(); random_array<8>();
    generic_values();
    std::printf("PASS pair reference checks=%zu\n", checks);
}
