// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_NEAREST_SOURCE
#define SIRKIT_NEAREST_SOURCE "../../../Geometry/Nearest_Two_points.cpp"
#endif
#include SIRKIT_NEAREST_SOURCE
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <random>

namespace {
std::atomic<std::ptrdiff_t> budget{-1}, live{0};
std::size_t checks = 0, failures = 0;
void check(bool ok) { ++checks; if (!ok) std::abort(); }
template<class Points> void check_zero(const Points& p) {
    const auto original = p;
    const auto inspect = [&](const auto& result) {
        check(result && result->distance_squared == 0);
        check(result->first_index < result->second_index && result->second_index < p.size());
        check(p[result->first_index] == p[result->second_index]);
    };
    inspect(nearest_two_points(p));
    inspect(nearest_two_points(p.begin(), p.end()));
    inspect(nearest_two_points(p.begin(), static_cast<std::ptrdiff_t>(p.size())));
    check(closest_pair(p.begin(), static_cast<int>(p.size())) == 0);
    check(p == original);
}
struct ReadFailure {};
struct Iterator {
    using value_type = P;
    using difference_type = std::ptrdiff_t;
    const std::vector<P>* points;
    std::size_t index;
    std::size_t fail_at;
    std::size_t* reads;
    const P& operator*() const {
        if (index == fail_at) throw ReadFailure{};
        ++*reads; return (*points)[index];
    }
    Iterator& operator++() { ++index; return *this; }
    void operator++(int) { ++*this; }
};
struct End { std::size_t n; };
bool operator==(const Iterator& it, End e) { return it.index == e.n; }
static_assert(std::input_iterator<Iterator> && std::sentinel_for<End, Iterator>);
void reference_cases() {
    std::mt19937 random(261022);
    for (std::size_t n : {2047u, 2048u, 2049u, 4096u}) {
        std::vector<P> points(n);
        for (auto& [x, y] : points) { x = static_cast<std::int32_t>(random()); y = static_cast<std::int32_t>(random()); }
        points[1] = points[0]; check_zero(points);
        std::fill(points.begin(), points.end(), P{INT32_MIN, INT32_MAX}); check_zero(points);
        for (std::size_t i = 0; i < n; ++i) points[i] = {static_cast<std::int32_t>(i), 0};
        points[n - 1] = points[n - 2]; check_zero(points);
        for (std::size_t i = 0; i < n; ++i) points[i] = {static_cast<std::int32_t>(i), 0};
        const auto unit = nearest_two_points(points);
        // 異なる整数格子点は距離二乗1以上、隣接点が上限1の証人。
        check(unit && unit->distance_squared == 1);
    }
}
void reads_and_failures() {
    const std::vector<P> points(4096, P{7, -19});
    const auto retained = live.load();
    for (std::size_t at : {0u, 1u, 2u, 2047u, 2048u, 4095u}) {
        std::size_t reads = 0; bool threw = false;
        try { (void)nearest_two_points(Iterator{&points, 0, at, &reads}, End{points.size()}); }
        catch (ReadFailure&) { threw = true; }
        check(threw && reads == at); check(live == retained);
    }
    for (bool unsized : {false, true}) {
        bool success = false;
        for (std::ptrdiff_t at = 0; at < 32; ++at) {
            std::size_t reads = 0;
            budget = at;
            try {
                const auto result = unsized
                    ? nearest_two_points(Iterator{&points, 0, points.size(), &reads}, End{points.size()})
                    : nearest_two_points(points);
                budget = -1;
                check(result && result->distance_squared == 0);
                if (unsized) check(reads == points.size());
                success = true;
            } catch (std::bad_alloc&) { budget = -1; ++failures; }
            check(live == retained);
            if (success) break;
        }
        check(success);
    }
#ifndef SKIP_RUNTIME_CHECKS
    for (auto bad : {INT64_C(2147483648), INT64_C(-2147483649)}) {
        std::vector<std::pair<std::int64_t, std::int64_t>> input(4096, {7, -19});
        input.back().first = bad;
        bool threw = false;
        try { (void)nearest_two_points(input); } catch (std::out_of_range&) { threw = true; }
        check(threw);
    }
#endif
}
}
void* operator new(std::size_t bytes) {
    const auto remaining = budget.load();
    if (remaining == 0) throw std::bad_alloc();
    if (remaining > 0) --budget;
    if (void* p = std::malloc(bytes ? bytes : 1)) { ++live; return p; }
    throw std::bad_alloc();
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* p) noexcept { if (p) --live; std::free(p); }
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { ::operator delete(p); }
int main() {
    const auto before = live.load(); reference_cases(); reads_and_failures(); check(live == before);
    std::printf("PASS checks=%zu failures=%zu live_delta=%td\n", checks, failures, live.load() - before);
}
