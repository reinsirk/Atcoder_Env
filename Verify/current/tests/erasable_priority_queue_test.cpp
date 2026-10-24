// SPDX-License-Identifier: CC0-1.0
// multisetとの独立照合。遅延削除、全API、所有権、値コピー例外を検査する。
#ifndef EPQ_SOURCE
#define EPQ_SOURCE "../../../DataStructure/erasable-priority-queue.cpp"
#endif
#include EPQ_SOURCE
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <random>
#include <set>
#include <source_location>
#include <string>
#include <utility>

static std::size_t checks = 0;
static void require(bool condition,
                    std::source_location loc = std::source_location::current()) {
    ++checks;
    if (!condition) {
        std::fprintf(stderr, "failed line %u\n", loc.line());
        std::abort();
    }
}

template <bool Minimize>
static auto first(std::multiset<std::int64_t>& values) {
    if constexpr (Minimize) return values.begin();
    else return std::prev(values.end());
}

template <bool Minimize>
static void validate(ErasablePriorityQueue<std::int64_t, Minimize>& queue,
                     const std::multiset<std::int64_t>& expected) {
    const auto& read_only = queue;
    require(read_only.size() == expected.size());
    require(read_only.empty() == expected.empty());
    if (expected.empty()) {
        require(!queue.try_top());
        require(!queue.extract_top());
    } else {
        const auto value = Minimize ? *expected.begin() : *expected.rbegin();
        require(queue.top() == value);
        require(queue.try_top() == value);
    }
}

template <bool Minimize>
static void drain(ErasablePriorityQueue<std::int64_t, Minimize>& queue,
                  std::multiset<std::int64_t> expected) {
    while (!expected.empty()) {
        auto it = first<Minimize>(expected);
        require(queue.extract_top() == *it);
        expected.erase(it);
    }
    validate(queue, expected);
}

template <bool Minimize>
static void ownership() {
    using Q = ErasablePriorityQueue<std::int64_t, Minimize>;
    const std::vector<std::int64_t> initial{7, -5, 7, 3, 0, 19, -9, 3};
    Q source(initial);
    source.erase(7);
    source.erase(3);
    std::multiset<std::int64_t> expected(initial.begin(), initial.end());
    expected.erase(expected.find(7)); expected.erase(expected.find(3));
    Q copy(source);
    Q assigned(std::vector<std::int64_t>{100, 200, 300});
    assigned.erase(200);
    assigned = source;
    Q* self = &assigned;
    assigned = *self;
    drain(copy, expected);
    Q moved(std::move(source));
    require(source.empty() && source.size() == 0);
    source.push(42); require(source.extract_top() == 42);
    Q move_assigned(std::vector<std::int64_t>{1, 2, 3});
    move_assigned.erase(2);
    move_assigned = std::move(moved);
    require(moved.empty());
    moved.emplace(-42); require(moved.extract_top() == -42);
    self = &move_assigned;
    move_assigned = std::move(*self);
    drain(move_assigned, expected);
    drain(assigned, expected);
    assigned.clear(); assigned.clear();
    assigned.emplace(8); require(assigned.top() == 8);
    assigned.clear(); require(assigned.empty());

    Q left(std::vector<std::int64_t>{1, 1, 4, 9});
    Q right(std::vector<std::int64_t>{-8, 6, 6, 12});
    left.erase(4); right.erase(6);
    static_assert(noexcept(left.swap(right)));
    left.swap(right);
    self = &left;
    left.swap(*self);
    Q empty;
    empty.swap(right);
    require(right.empty() && right.size() == 0);
    drain(left, {-8, 6, 12});
    drain(empty, {1, 1, 9});
    right.emplace(27); require(right.extract_top() == 27);
}

template <bool Minimize>
static void randomized() {
    using Q = ErasablePriorityQueue<std::int64_t, Minimize>;
    constexpr auto lo = std::numeric_limits<std::int64_t>::min();
    constexpr auto hi = std::numeric_limits<std::int64_t>::max();
    std::mt19937_64 rng(20261022 + Minimize);
    for (int round = 0; round < 12; ++round) {
        const std::vector<std::int64_t> initial{lo, hi, 0, 0, -1, 1, lo, hi};
        Q queue(initial);
        std::multiset<std::int64_t> expected(initial.begin(), initial.end());
        for (int step = 0; step < 12000; ++step) {
            const auto action = rng() % 11;
            if (expected.empty() || action < 4) {
                const auto bits = rng();
                const std::int64_t value = (bits % 8 == 0) ? lo :
                    (bits % 8 == 1) ? hi : static_cast<std::int64_t>(bits % 33) - 16;
                if (step % 3 == 0) queue.push(value);
                else if (step % 3 == 1) queue.push(std::int64_t(value));
                else queue.emplace(value);
                expected.insert(value);
            } else if (action < 7) {
                auto it = expected.begin();
                std::advance(it, rng() % expected.size());
                queue.erase(*it); expected.erase(it);
            } else if (action < 10) {
                auto it = first<Minimize>(expected);
                if (step & 1) {
                    require(queue.top() == *it); queue.pop();
                } else require(queue.extract_top() == *it);
                expected.erase(it);
            } else {
                validate(queue, expected);
            }
            // 毎回normalizeせず、削除が溜まった状態も照合する。
            require(queue.size() == expected.size());
            require(queue.empty() == expected.empty());
            if (step % 97 == 0) validate(queue, expected);
            if (step % 997 == 0) {
                Q copy(queue); drain(copy, expected);
            }
        }
        drain(queue, expected);
    }
}

template <bool Minimize>
static void delayed_deletions() {
    using Q = ErasablePriorityQueue<std::int64_t, Minimize>;
    Q queue;
    std::multiset<std::int64_t> expected;
    for (int i = 0; i < 50000; ++i) { queue.emplace(i); expected.insert(i); }
    for (int i = 0; i < 50000; i += 2) {
        queue.erase(i); expected.erase(expected.find(i));
    }
    validate(queue, expected);
    drain(queue, expected);
    for (int i = 0; i < 20000; ++i) queue.push(i % 5);
    for (int i = 0; i < 20000; ++i) queue.erase(i % 5);
    validate(queue, {});
    queue.push(10); require(queue.top() == 10);
    require(queue.extract_top() == 10);
    validate(queue, {});
}

template <class T, bool Minimize>
static void arithmetic_types() {
    std::vector<T> values{T{}, T{1}, std::numeric_limits<T>::lowest(),
                          std::numeric_limits<T>::max(), T{}, T{1}};
    if constexpr (std::is_floating_point_v<T>) {
        values.push_back(-T{});
        values.push_back(std::numeric_limits<T>::min());
        values.push_back(std::numeric_limits<T>::denorm_min());
        values.push_back(std::numeric_limits<T>::infinity());
        values.push_back(-std::numeric_limits<T>::infinity());
    }
    ErasablePriorityQueue<T, Minimize> queue(values);
    std::multiset<T> expected(values.begin(), values.end());
    std::mt19937 rng(20261022);
    for (int i = 0; i < 3000; ++i) {
        if (expected.empty() || rng() % 3 == 0) {
            const T value = static_cast<T>(rng() % 10);
            queue.push(value); expected.insert(value);
        } else if (rng() & 1) {
            auto it = expected.begin(); std::advance(it, rng() % expected.size());
            queue.erase(*it); expected.erase(it);
        } else {
            auto it = Minimize ? expected.begin() : std::prev(expected.end());
            require(queue.extract_top() == *it); expected.erase(it);
        }
        require(queue.size() == expected.size());
        if (!expected.empty())
            require(queue.top() == (Minimize ? *expected.begin() : *expected.rbegin()));
    }
    while (!expected.empty()) {
        auto it = Minimize ? expected.begin() : std::prev(expected.end());
        require(queue.extract_top() == *it); expected.erase(it);
    }
    require(queue.empty() && !queue.try_top());
}

template <class T>
static void both_arithmetic_directions() {
    arithmetic_types<T, true>(); arithmetic_types<T, false>();
}

template <bool Minimize>
static void aliased_push() {
    using T = std::int64_t;
    using Q = ErasablePriorityQueue<T, Minimize>;
    for (const std::size_t n : {1U, 2U, 3U, 8U, 16U, 63U, 64U, 65U}) {
        for (const bool spare : {false, true}) {
            for (const std::size_t index : {std::size_t{0}, n / 2, n - 1}) {
                for (int operation = 0; operation != 3; ++operation) {
                    std::vector<T> values;
                    values.reserve(n + (spare ? 7 : 0));
                    for (std::size_t i = 0; i < n; ++i)
                        values.push_back(static_cast<T>((i * 1009 + 17) % 127) - 63);
                    // 満杯時は次のpushで再確保が必要になる。
                    if (!spare)
                        while (values.size() < values.capacity()) values.push_back(19);
                    std::multiset<T> expected(values.begin(), values.end());
                    T* const alias = &values[index];
                    Q queue(std::move(values));
                    // vectorの移譲後も参照は有効。heap化後の値を記録する。
                    const T inserted = *alias;
                    if (operation == 0) queue.push(std::as_const(*alias));
                    else if (operation == 1) queue.push(std::move(*alias));
                    else queue.emplace(*alias);
                    // 再確保後にaliasを参照しない。
                    expected.insert(inserted);
                    drain(queue, expected);
                }
            }
        }
    }
}

template <class T, bool Minimize>
static void emplace_conversions() {
    ErasablePriorityQueue<T, Minimize> queue;
    std::multiset<T> expected;
    queue.emplace(); expected.insert(T{});
    queue.emplace(1.75); expected.insert(static_cast<T>(1.75));
    queue.emplace(-0.25); expected.insert(static_cast<T>(-0.25));
    queue.emplace(false); expected.insert(static_cast<T>(false));
    queue.emplace(true); expected.insert(static_cast<T>(true));
    while (!expected.empty()) {
        auto it = Minimize ? expected.begin() : std::prev(expected.end());
        require(queue.extract_top() == *it); expected.erase(it);
    }
    require(queue.empty());
}

struct ConversionFailure {};
struct IntegerSource {
    std::int64_t value;
    bool fail = false;
    explicit operator std::int64_t() const {
        if (fail) throw ConversionFailure{};
        return value;
    }
};

template <bool Minimize>
static void explicit_integer_conversion() {
    ErasablePriorityQueue<std::int64_t, Minimize> queue;
    queue.emplace(IntegerSource{-17});
    queue.emplace(IntegerSource{23});
    require(queue.extract_top() == (Minimize ? -17 : 23));
    require(queue.extract_top() == (Minimize ? 23 : -17));
    require(queue.empty());
    for (const bool spare : {false, true}) {
        std::vector<std::int64_t> values;
        values.reserve(spare ? 16 : 5);
        for (const auto value : {-9, -1, 5, 7, 11}) values.push_back(value);
        if (!spare)
            while (values.size() < values.capacity()) values.push_back(13);
        std::multiset<std::int64_t> expected(values.begin(), values.end());
        ErasablePriorityQueue<std::int64_t, Minimize> pending(std::move(values));
        pending.erase(5); expected.erase(expected.find(5));
        bool caught = false;
        try { pending.emplace(IntegerSource{99, true}); }
        catch (const ConversionFailure&) { caught = true; }
        require(caught);
        require(pending.size() == expected.size());
        drain(pending, expected);
    }
}

template <class T>
static void both_emplace_directions() {
    emplace_conversions<T, true>(); emplace_conversions<T, false>();
}

struct CopyFailure {};
struct Value {
    static inline int live = 0, copies_left = -1;
    int key;
    std::string payload;
    explicit Value(int n) : key(n), payload(80, char('a' + (n % 20 + 20) % 20)) { ++live; }
    Value(const Value& other) : key(other.key), payload(other.payload) {
        if (copies_left == 0) throw CopyFailure{};
        if (copies_left > 0) --copies_left;
        ++live;
    }
    Value(Value&& other) noexcept : key(other.key), payload(std::move(other.payload)) { ++live; }
    Value& operator=(const Value&) = default;
    Value& operator=(Value&&) noexcept = default;
    ~Value() { --live; }
    friend bool operator<(const Value& a, const Value& b) { return a.key < b.key; }
    friend bool operator>(const Value& a, const Value& b) { return a.key > b.key; }
    friend bool operator==(const Value& a, const Value& b) { return a.key == b.key; }
};

template <class F>
static void copy_failure(F&& f) {
    bool caught = false;
    Value::copies_left = 0;
    try { f(); } catch (const CopyFailure&) { caught = true; }
    Value::copies_left = -1;
    require(caught);
}

template <bool Minimize>
static void nontrivial_values() {
    using Q = ErasablePriorityQueue<Value, Minimize>;
    {
        Q queue;
        for (int i : {4, -2, 7, 4, 1}) queue.emplace(i);
        queue.erase(Value(4));
        require(queue.size() == 4);
        Q copy(queue);
        Q moved(std::move(copy));
        require(copy.empty());
        const std::vector<int> order = Minimize ? std::vector<int>{-2, 1, 4, 7}
                                               : std::vector<int>{7, 4, 1, -2};
        for (const int key : order) {
            require(moved.try_top()->key == key);
            require(moved.extract_top()->key == key);
        }
        require(moved.empty());
        const int before = Value::live;
        copy_failure([&] { (void)queue.top(); });
        require(queue.size() == 4 && Value::live == before);
        copy_failure([&] { (void)queue.try_top(); });
        require(queue.size() == 4 && Value::live == before);
        copy_failure([&] { (void)queue.extract_top(); });
        require(queue.size() == 4 && Value::live == before);
        Value pending(4);
        copy_failure([&] { queue.erase(pending); });
        require(queue.size() == 4);
        copy_failure([&] { Q failed(queue); });
        require(queue.size() == 4);
        for (const int key : order) require(queue.extract_top()->key == key);
        queue.clear();

        Q other;
        queue.emplace(-3); queue.emplace(9); queue.erase(Value(9));
        other.emplace(2); other.emplace(7); other.erase(Value(2));
        // 交換は要素のコピーを行わず、遅延削除も一緒に移す。
        Value::copies_left = 0;
        queue.swap(other);
        Value::copies_left = -1;
        require(queue.extract_top()->key == 7);
        require(other.extract_top()->key == -3);
        require(queue.empty() && other.empty());
    }
    require(Value::live == 0);
}

#ifndef SKIP_RUNTIME_CHECKS
template <class F>
static void empty_failure(F&& f) {
    bool caught = false;
    try { f(); } catch (const std::out_of_range&) { caught = true; }
    require(caught);
}
static void invalid_empty_access() {
    ErasablePriorityQueue<int> queue;
    empty_failure([&] { queue.top(); });
    empty_failure([&] { queue.pop(); });
    empty_failure([&] { queue.erase(1); });
    queue.push(1); queue.erase(1);
    empty_failure([&] { queue.top(); });
    empty_failure([&] { queue.pop(); });
    empty_failure([&] { queue.erase(1); });
    require(queue.empty());
    queue.push(2); require(queue.top() == 2);
}
#endif

int main() {
    ownership<true>(); ownership<false>();
    randomized<true>(); randomized<false>();
    delayed_deletions<true>(); delayed_deletions<false>();
    both_arithmetic_directions<bool>();
    both_arithmetic_directions<std::int8_t>();
    both_arithmetic_directions<std::uint8_t>();
    both_arithmetic_directions<std::int16_t>();
    both_arithmetic_directions<std::uint16_t>();
    both_arithmetic_directions<int>();
    both_arithmetic_directions<unsigned>();
    both_arithmetic_directions<std::uint64_t>();
    both_arithmetic_directions<float>();
    both_arithmetic_directions<double>();
    both_arithmetic_directions<long double>();
    aliased_push<true>(); aliased_push<false>();
    both_emplace_directions<bool>();
    both_emplace_directions<std::int8_t>();
    both_emplace_directions<std::uint8_t>();
    both_emplace_directions<std::int16_t>();
    both_emplace_directions<std::uint16_t>();
    both_emplace_directions<int>();
    both_emplace_directions<unsigned>();
    both_emplace_directions<std::int64_t>();
    both_emplace_directions<std::uint64_t>();
    explicit_integer_conversion<true>(); explicit_integer_conversion<false>();
    nontrivial_values<true>(); nontrivial_values<false>();
#ifndef SKIP_RUNTIME_CHECKS
    invalid_empty_access();
#endif
    std::printf("PASS %zu checks; live values %d\n", checks, Value::live);
}
