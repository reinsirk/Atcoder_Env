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

// bitwise_and_convolution(f,g): h[k]=sum_{i&j=k}f[i]g[j]。同じ長さ2^k、O(n log n)。
// bitwise_or_convolution(f,g): h[k]=sum_{i|j=k}f[i]g[j]。同じ長さ2^k、O(n log n)。
#ifndef REINSIRK_BITWISE_AND_CONVOLUTION_DEFINED
#define REINSIRK_BITWISE_AND_CONVOLUTION_DEFINED
template <class T> std::vector<T> bitwise_and_convolution(std::vector<T> f, std::vector<T> g) {
#ifndef SKIP_RUNTIME_CHECKS
    if (f.size() != g.size()) throw std::invalid_argument("bitwise_and_convolution: different lengths");
#endif
    superset_zeta_transform(f);
    superset_zeta_transform(g);
    for (std::size_t i = 0; i < f.size(); ++i) f[i] *= g[i];
    superset_mobius_transform(f);
    return f;
}
#endif

#ifndef REINSIRK_BITWISE_OR_CONVOLUTION_DEFINED
#define REINSIRK_BITWISE_OR_CONVOLUTION_DEFINED
template <class T> std::vector<T> bitwise_or_convolution(std::vector<T> f, std::vector<T> g) {
#ifndef SKIP_RUNTIME_CHECKS
    if (f.size() != g.size()) throw std::invalid_argument("bitwise_or_convolution: different lengths");
#endif
    subset_zeta_transform(f);
    subset_zeta_transform(g);
    for (std::size_t i = 0; i < f.size(); ++i) f[i] *= g[i];
    subset_mobius_transform(f);
    return f;
}
#endif


