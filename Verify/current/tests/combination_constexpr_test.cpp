// SPDX-License-Identifier: CC0-1.0
#ifndef COMBINATION_HEADER
#define COMBINATION_HEADER "../../../Algorithm/next_combination.cpp"
#endif
#include COMBINATION_HEADER

#include <bit>
#include <cstdint>
#include <limits>

namespace {
// ビット位置から端点を作り、列挙順・個数・ビット数を独立に確認する。
template <class UInt>
constexpr bool CheckCase(int n, int k) {
    UInt first = 0;
    UInt last = 0;
    UInt allowed = 0;
    for (int bit = 0; bit < n; ++bit) {
        const auto value = static_cast<UInt>(UInt{1} << bit);
        allowed = static_cast<UInt>(allowed | value);
        if (bit < k) first = static_cast<UInt>(first | value);
        if (bit >= n - k) last = static_cast<UInt>(last | value);
    }
    if (first_combination<UInt>(k) != first || last_combination<UInt>(n, k) != last)
        return false;

    std::uint64_t expected_count = 1;
    const int r = k < n - k ? k : n - k;
    for (int i = 1; i <= r; ++i)
        expected_count = expected_count * static_cast<std::uint64_t>(n - r + i)
                         / static_cast<std::uint64_t>(i);

    bool valid = true;
    UInt previous = 0;
    std::uint64_t count = 0;
    for_each_combination<UInt>(n, k, [&](UInt mask) constexpr {
        if (std::popcount(mask) != k || (mask & static_cast<UInt>(~allowed)) != 0)
            valid = false;
        if (count == 0 ? mask != first : mask <= previous) valid = false;
        previous = mask;
        ++count;
    });
    return valid && count == expected_count && previous == last;
}

template <class UInt>
constexpr bool CheckSmallCases() {
    for (int n = 0; n <= 8; ++n)
        for (int k = 0; k <= n; ++k)
            if (!CheckCase<UInt>(n, k)) return false;
    return true;
}

template <class UInt>
constexpr bool CheckBoundaryCases() {
    constexpr int width = std::numeric_limits<UInt>::digits;
    return CheckCase<UInt>(width, 0) && CheckCase<UInt>(width, 1)
        && CheckCase<UInt>(width, width - 1) && CheckCase<UInt>(width, width)
        && CheckCase<UInt>(width - 1, 0) && CheckCase<UInt>(width - 1, 1)
        && CheckCase<UInt>(width - 1, width - 2)
        && CheckCase<UInt>(width - 1, width - 1);
}

static_assert(CheckSmallCases<std::uint8_t>());
static_assert(CheckSmallCases<std::uint16_t>());
static_assert(CheckSmallCases<std::uint32_t>());
static_assert(CheckSmallCases<std::uint64_t>());
static_assert(CheckBoundaryCases<std::uint8_t>());
static_assert(CheckBoundaryCases<std::uint16_t>());
static_assert(CheckBoundaryCases<std::uint32_t>());
static_assert(CheckBoundaryCases<std::uint64_t>());
static_assert(CheckCase<std::uint8_t>(8, 2));
static_assert(CheckCase<std::uint16_t>(16, 2));
static_assert(CheckCase<std::uint32_t>(32, 2));
static_assert(CheckCase<std::uint64_t>(64, 2));
static_assert(CheckCase<std::uint32_t>(31, 2));
static_assert(CheckCase<std::uint64_t>(63, 2));
} // namespace

int main() {}
