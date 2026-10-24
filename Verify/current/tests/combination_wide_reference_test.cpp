// SPDX-License-Identifier: CC0-1.0
#ifndef COMBINATION_HEADER
#define COMBINATION_HEADER "../../../Algorithm/next_combination.cpp"
#endif
#include COMBINATION_HEADER
#include COMBINATION_HEADER

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>

namespace {
std::size_t checks = 0;

// 1の位置を列挙し、増やせる最下位の位置を繰り上げる独立参照。
template <class UInt>
constexpr bool reference_next(UInt& value, int n) {
    std::array<int, std::numeric_limits<UInt>::digits> position{};
    int count = 0;
    for (int bit = 0; bit < n; ++bit)
        if ((value >> bit) & UInt{1}) position[count++] = bit;
    for (int i = 0; i < count; ++i) {
        const int limit = i + 1 == count ? n : position[i + 1];
        if (position[i] + 1 == limit) continue;
        ++position[i];
        for (int j = 0; j < i; ++j) position[j] = j;
        UInt result = 0;
        for (int j = 0; j < count; ++j)
            result |= UInt{1} << position[j];
        value = result;
        return true;
    }
    return false;
}

template <class UInt>
constexpr UInt reference_first(int k, int offset = 0) {
    UInt value = 0;
    for (int bit = offset; bit < offset + k; ++bit)
        value |= UInt{1} << bit;
    return value;
}

static_assert([] {
    std::uint64_t value = (UINT64_C(1) << 63) | (UINT64_C(1) << 31) | 15;
    auto expected = value;
    return reference_next(expected, 64) && next_combination(value) == expected;
}());
static_assert([] {
    auto value = UINT32_C(0x7fffffff);
    return next_combination(value, 32) && value == UINT32_C(0xbfffffff);
}());

template <class UInt>
void check_mask(UInt mask, int n) {
    constexpr int width = std::numeric_limits<UInt>::digits;
    UInt expected = mask;
    const bool has_next = reference_next(expected, n);
    UInt actual = mask;
    assert(next_combination(actual, n) == has_next);
    assert(actual == expected);  // 終端時の不変性も確認。
    expected = mask;
    const bool has_full_next = reference_next(expected, width);
    assert(next_combination(mask) == (has_full_next ? expected : UInt{0}));
    ++checks;
}

template <class UInt>
void check_width(std::mt19937_64& random) {
    constexpr int width = std::numeric_limits<UInt>::digits;
    for (int n : {17, 30, 31, 32, 33, 62, 63, 64}) {
        if (n > width) continue;
        std::array<int, width> positions{};
        std::iota(positions.begin(), positions.end(), 0);
        for (int k = 0; k <= n; ++k) {
            const UInt first = reference_first<UInt>(k);
            const UInt last = reference_first<UInt>(k, n - k);
            assert(first_combination<UInt>(k) == first);
            assert(last_combination<UInt>(n, k) == last);
            check_mask(first, n);
            check_mask(last, n);
            // 長い連続1列が上位へ繰り上がる境界を全位置で確認。
            for (int offset = 0; offset <= n - k; ++offset)
                check_mask(reference_first<UInt>(k, offset), n);
            for (int repetition = 0; repetition < 64; ++repetition) {
                std::shuffle(positions.begin(), positions.begin() + n, random);
                UInt mask = 0;
                for (int i = 0; i < k; ++i) mask |= UInt{1} << positions[i];
                check_mask(mask, n);
            }
        }
        // 大幅でも列挙数がO(n^2)に収まる両側の個数を独立参照と照合。
        for (int k : {2, n - 2}) {
            UInt expected = reference_first<UInt>(k);
            std::size_t visits = 0;
            bool exhausted = false;
            for_each_combination<UInt>(n, k, [&](UInt value) {
                assert(!exhausted && value == expected);
                ++visits;
                exhausted = !reference_next(expected, n);
                value = 0;  // コールバック引数の変更は列挙に影響しない。
            });
            assert(exhausted);
            assert(visits == static_cast<std::size_t>(n) * (n - 1) / 2);
            ++checks;
        }
    }
    int visits = 0;
    bool caught = false;
    try {
        for_each_combination<UInt>(width, 2, [&](UInt) {
            if (++visits == 37) throw 123;
        });
    } catch (int value) {
        caught = value == 123;
    }
    assert(caught && visits == 37);
    ++checks;
#ifndef SKIP_RUNTIME_CHECKS
    const auto rejects = [](auto operation) {
        bool caught = false;
        try { operation(); }
        catch (const std::out_of_range&) { caught = true; }
        assert(caught);
        ++checks;
    };
    rejects([&] { (void)first_combination<UInt>(width + 1); });
    rejects([&] { (void)last_combination<UInt>(width + 1, 2); });
    rejects([&] { (void)last_combination<UInt>(width, width + 1); });
    for (int n : {-1, width + 1}) {
        UInt mask = 1;
        rejects([&] { (void)next_combination(mask, n); });
        assert(mask == 1);
    }
    UInt outside = UInt{1} << (width - 1);
    const auto original = outside;
    rejects([&] { (void)next_combination(outside, width - 1); });
    assert(outside == original);
    rejects([&] { for_each_combination<UInt>(width + 1, 2, [](UInt) { assert(false); }); });
    rejects([&] { for_each_combination<UInt>(width, -1, [](UInt) { assert(false); }); });
#endif
}

void check_legacy(std::mt19937_64& random) {
    constexpr int width = std::numeric_limits<int>::digits;
    std::array<int, width> positions{};
    std::iota(positions.begin(), positions.end(), 0);
    for (int k = 0; k <= width; ++k) {
        for (int repetition = 0; repetition < 128; ++repetition) {
            std::shuffle(positions.begin(), positions.end(), random);
            unsigned int mask = 0;
            for (int i = 0; i < k; ++i) mask |= 1U << positions[i];
            auto expected = mask;
            const bool advanced = reference_next(expected, width);
            assert(next_combination(static_cast<int>(mask)) ==
                   (advanced ? static_cast<int>(expected) : 0));
            ++checks;
        }
        for (int offset = 0; offset <= width - k; ++offset) {
            const auto mask = reference_first<unsigned int>(k, offset);
            auto expected = mask;
            const bool advanced = reference_next(expected, width);
            assert(next_combination(static_cast<int>(mask)) ==
                   (advanced ? static_cast<int>(expected) : 0));
            ++checks;
        }
    }
}
} // namespace

int main() {
    std::mt19937_64 random(UINT64_C(0x171064320012));
    check_width<std::uint32_t>(random);
    check_width<std::uint64_t>(random);
    check_legacy(random);
    std::cout << "PASS wide position-reference checks=" << checks << '\n';
}
