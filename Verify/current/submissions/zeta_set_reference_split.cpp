// Verify: ローカル独立検査

// Library: Math/Zeta/Set_Transform.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/set-function/zeta-mobius-transform.hpp.html
#include <bit>
#include <cstddef>
#include <stdexcept>
#include <vector>

// 集合変換は空配列または長さ2^k。各演算はO(n log n)、追加領域O(1)。
// superset_zeta_transform(f): f[S]をSの上位集合の総和へ更新する。
// superset_mobius_transform(f): 上位集合ゼータ変換を元に戻す。
// subset_zeta_transform(f): f[S]をSの部分集合の総和へ更新する。
// subset_mobius_transform(f): 部分集合ゼータ変換を元に戻す。
#ifndef REINSIRK_SUPERSET_ZETA_TRANSFORM_DEFINED
#define REINSIRK_SUPERSET_ZETA_TRANSFORM_DEFINED
template <class T> void superset_zeta_transform(std::vector<T>& f) {
#ifndef SKIP_RUNTIME_CHECKS
    if (!f.empty() && !std::has_single_bit(f.size()))
        throw std::invalid_argument("superset_zeta_transform: length must be a power of two");
#endif
    for (std::size_t half = 1; half < f.size(); half *= 2) {
        for (std::size_t base = 0; base < f.size(); base += half * 2) {
            for (std::size_t j = 0; j < half; ++j) f[base + j] += f[base + half + j];
        }
    }
}
#endif

#ifndef REINSIRK_SUPERSET_MOBIUS_TRANSFORM_DEFINED
#define REINSIRK_SUPERSET_MOBIUS_TRANSFORM_DEFINED
template <class T> void superset_mobius_transform(std::vector<T>& f) {
#ifndef SKIP_RUNTIME_CHECKS
    if (!f.empty() && !std::has_single_bit(f.size()))
        throw std::invalid_argument("superset_mobius_transform: length must be a power of two");
#endif
    for (std::size_t half = 1; half < f.size(); half *= 2) {
        for (std::size_t base = 0; base < f.size(); base += half * 2) {
            for (std::size_t j = 0; j < half; ++j) f[base + j] -= f[base + half + j];
        }
    }
}
#endif

#ifndef REINSIRK_SUBSET_ZETA_TRANSFORM_DEFINED
#define REINSIRK_SUBSET_ZETA_TRANSFORM_DEFINED
template <class T> void subset_zeta_transform(std::vector<T>& f) {
#ifndef SKIP_RUNTIME_CHECKS
    if (!f.empty() && !std::has_single_bit(f.size()))
        throw std::invalid_argument("subset_zeta_transform: length must be a power of two");
#endif
    for (std::size_t half = 1; half < f.size(); half *= 2) {
        for (std::size_t base = 0; base < f.size(); base += half * 2) {
            for (std::size_t j = 0; j < half; ++j) f[base + half + j] += f[base + j];
        }
    }
}
#endif

#ifndef REINSIRK_SUBSET_MOBIUS_TRANSFORM_DEFINED
#define REINSIRK_SUBSET_MOBIUS_TRANSFORM_DEFINED
template <class T> void subset_mobius_transform(std::vector<T>& f) {
#ifndef SKIP_RUNTIME_CHECKS
    if (!f.empty() && !std::has_single_bit(f.size()))
        throw std::invalid_argument("subset_mobius_transform: length must be a power of two");
#endif
    for (std::size_t half = 1; half < f.size(); half *= 2) {
        for (std::size_t base = 0; base < f.size(); base += half * 2) {
            for (std::size_t j = 0; j < half; ++j) f[base + half + j] -= f[base + j];
        }
    }
}
#endif



// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::mt19937 generator(71234);
    for (int exponent = 0; exponent <= 8; ++exponent) {
        const int n = 1 << exponent;
        std::vector<std::int64_t> a(n);
        for (auto& x : a) x = static_cast<int>(generator() % 101) - 50;
        for (bool upper : {false, true}) {
            auto expected = a;
            for (int i = 0; i < n; ++i) {
                expected[i] = 0;
                for (int j = 0; j < n; ++j)
                    if (upper ? (i & j) == i : (i & j) == j) expected[i] += a[j];
            }
            auto actual = a;
            if (upper) superset_zeta_transform(actual); else subset_zeta_transform(actual);
            assert(actual == expected);
            if (upper) superset_mobius_transform(actual); else subset_mobius_transform(actual);
            assert(actual == a);
        }
    }
    std::vector<int> empty;
    subset_zeta_transform(empty); subset_mobius_transform(empty);
    superset_zeta_transform(empty); superset_mobius_transform(empty);
    std::cout << "OK\n";
}
