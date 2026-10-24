// SPDX-License-Identifier: CC0-1.0
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#ifndef SIRKIT_TEST_TOPOLOGICAL_SOURCE
#define SIRKIT_TEST_TOPOLOGICAL_SOURCE "../../../Graph/Topological.cpp"
#endif
#include SIRKIT_TEST_TOPOLOGICAL_SOURCE

namespace {
std::size_t cases = 0, checks = 0;

std::uint64_t reference(const std::uint32_t* degree) {
    // 上位から二進数を組み立て、各要素のゼロ判定だけを使う。
    std::uint64_t mask = 0;
    for (std::size_t i = 64; i-- != 0;)
        mask = mask * 2 + static_cast<std::uint64_t>(degree[i] == 0);
    return mask;
}

void check(const std::array<std::uint32_t, 64>& values, bool avx2) {
    // 32ビット境界を保ちつつ、32バイト境界外の読み込みも試す。
    std::array<std::uint32_t, 65> storage{};
    for (std::size_t offset : {std::size_t{0}, std::size_t{1}}) {
        for (std::size_t i = 0; i < 64; ++i) storage[offset + i] = values[i];
        const auto* degree = storage.data() + offset;
        const auto expected = reference(degree);
        ++cases;
        ++checks;
        if (reinsirk::detail::TopologicalReadyMask(degree) != expected)
            throw std::runtime_error("topological scalar ready mask mismatch");
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))
        if (avx2) {
            ++checks;
            if (reinsirk::detail::TopologicalReadyMaskAvx2(degree) != expected)
                throw std::runtime_error("topological AVX2 ready mask mismatch");
        }
#else
        (void)avx2;
#endif
    }
}
} // namespace

int main() {
    try {
        bool avx2 = false;
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))
        avx2 = __builtin_cpu_supports("avx2");
#endif
        std::array<std::uint32_t, 64> values{};
        check(values, avx2);
        values.fill(1);
        check(values, avx2);
        for (std::size_t i = 0; i < 64; ++i) {
            values.fill(std::numeric_limits<std::uint32_t>::max());
            values[i] = 0;
            check(values, avx2);
            values.fill(0);
            values[i] = std::uint32_t{1} << (i % 32);
            check(values, avx2);
        }
        for (unsigned period : {2U, 3U, 7U, 8U, 11U, 31U, 32U}) {
            for (std::size_t i = 0; i < 64; ++i)
                values[i] = i % period == 0 ? 0 : std::numeric_limits<std::uint32_t>::max();
            check(values, avx2);
        }
        std::mt19937 rng(0x71509C23);
        for (unsigned trial = 0; trial < 1000; ++trial) {
            for (auto& value : values) {
                const auto random = static_cast<std::uint32_t>(rng());
                value = random % (trial % 11 + 2) == 0 ? 0 : random;
            }
            check(values, avx2);
        }
        std::cout << "{\"cases\":" << cases << ",\"checks\":" << checks
                  << ",\"avx2\":" << avx2 << "}\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
