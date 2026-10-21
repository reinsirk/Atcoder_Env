// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/set-function/zeta-mobius-transform.hpp.html
#include <bit>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

// 集合変換は空配列または長さ2^k。各演算はO(n log n)、追加領域O(1)。
// superset_zeta_transform(f): f[S]をSの上位集合の総和へ更新する。
// superset_mobius_transform(f): 上位集合ゼータ変換を元に戻す。
// subset_zeta_transform(f): f[S]をSの部分集合の総和へ更新する。
// subset_mobius_transform(f): 部分集合ゼータ変換を元に戻す。
#ifndef SIRKIT_SET_TRANSFORM_SIMD_DETAIL_DEFINED
#define SIRKIT_SET_TRANSFORM_SIMD_DETAIL_DEFINED
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
namespace reinsirk::detail {
template <bool Subset, bool Subtract, std::size_t Half, class T>
[[gnu::always_inline]] inline void set_transform_small_stage(T* f, std::size_t n) {
    for (std::size_t base=0; base<n; base+=Half*2) {
        T* out=f+base+(Subset?Half:0);
        const T* in=f+base+(Subset?0:Half);
        for (std::size_t j=0;j<Half;++j) {
            if constexpr(Subtract) out[j]-=in[j];else out[j]+=in[j];
        }
    }
}
template <bool Subset, bool Subtract, class T>
[[gnu::always_inline]] inline void set_transform_small(T* f,std::size_t n) {
    if(n>=2)set_transform_small_stage<Subset,Subtract,1>(f,n);
    if(n>=4)set_transform_small_stage<Subset,Subtract,2>(f,n);
    if(n>=8)set_transform_small_stage<Subset,Subtract,4>(f,n);
    if(n>=16)set_transform_small_stage<Subset,Subtract,8>(f,n);
    if(n>=32)set_transform_small_stage<Subset,Subtract,16>(f,n);
}
template <bool Subset, bool Subtract, unsigned Bit, class U, class V, std::size_t... I>
[[gnu::target("avx2"),gnu::always_inline]] inline V set_transform_stage(V v, std::index_sequence<I...>) {
    const V swapped = __builtin_shufflevector(v, v, (I ^ Bit)...);
    const V mask{static_cast<U>((static_cast<bool>(I & Bit) == Subset) ? static_cast<U>(~U{0}) : U{0})...};
    if constexpr (Subtract) v -= swapped & mask;
    else v += swapped & mask;
    if constexpr (Bit * 2 < sizeof(V) / sizeof(U))
        return set_transform_stage<Subset,Subtract,Bit*2,U>(v, std::index_sequence<I...>{});
    return v;
}
template <bool Subset, bool Subtract, class T>
[[gnu::target("avx2")]] void set_transform_simd(T* f, std::size_t n) {
    using U = std::make_unsigned_t<T>;
    typedef U V __attribute__((vector_size(32)));
    constexpr std::size_t width = sizeof(V) / sizeof(T);
    for (std::size_t base = 0; base < n; base += width) {
        V v;
        std::memcpy(&v, f + base, sizeof(v));
        v = set_transform_stage<Subset,Subtract,1,U>(v, std::make_index_sequence<width>{});
        std::memcpy(f + base, &v, sizeof(v));
    }
    for (std::size_t half = width; half < n; half *= 2) {
        for (std::size_t base = 0; base < n; base += half * 2) {
            T* out = f + base + (Subset ? half : 0);
            const T* in = f + base + (Subset ? 0 : half);
            for (std::size_t j = 0; j < half; j += width) {
                V a, b;
                std::memcpy(&a, out + j, sizeof(a));
                std::memcpy(&b, in + j, sizeof(b));
                if constexpr (Subtract) a -= b;
                else a += b;
                std::memcpy(out + j, &a, sizeof(a));
            }
        }
    }
}
}
#endif
#endif


#ifndef SIRKIT_SUPERSET_ZETA_TRANSFORM_DEFINED
#define SIRKIT_SUPERSET_ZETA_TRANSFORM_DEFINED
template <class T> void superset_zeta_transform(std::vector<T>& f) {
#ifndef SKIP_RUNTIME_CHECKS
    if (!f.empty() && !std::has_single_bit(f.size()))
        throw std::invalid_argument("superset_zeta_transform: length must be a power of two");
#endif
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T,bool> && sizeof(T)<=8) {
        if (f.size()>=8) {
        if(f.size()==8){f[0]+=f[1];f[2]+=f[3];f[4]+=f[5];f[6]+=f[7];f[0]+=f[2];f[1]+=f[3];f[4]+=f[6];f[5]+=f[7];f[0]+=f[4];f[1]+=f[5];f[2]+=f[6];f[3]+=f[7];return;}
        if (f.size()<64) {
            switch(f.size()) {
            case 16:reinsirk::detail::set_transform_small<false,false>(f.data(),16);return;
            case 32:reinsirk::detail::set_transform_small<false,false>(f.data(),32);return;
            default:return;
            }
        }
        if (__builtin_cpu_supports("avx2")) {
            reinsirk::detail::set_transform_simd<false,false>(f.data(), f.size());
            return;
        }
        }
    }
#endif
    for (std::size_t half = 1; half < f.size(); half *= 2) {
        for (std::size_t base = 0; base < f.size(); base += half * 2) {
            for (std::size_t j = 0; j < half; ++j) f[base + j] += f[base + half + j];
        }
    }
}
#endif

#ifndef SIRKIT_SUPERSET_MOBIUS_TRANSFORM_DEFINED
#define SIRKIT_SUPERSET_MOBIUS_TRANSFORM_DEFINED
template <class T> void superset_mobius_transform(std::vector<T>& f) {
#ifndef SKIP_RUNTIME_CHECKS
    if (!f.empty() && !std::has_single_bit(f.size()))
        throw std::invalid_argument("superset_mobius_transform: length must be a power of two");
#endif
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T,bool> && sizeof(T)<=8) {
        if (f.size()>=8) {
        if(f.size()==8){f[0]-=f[1];f[2]-=f[3];f[4]-=f[5];f[6]-=f[7];f[0]-=f[2];f[1]-=f[3];f[4]-=f[6];f[5]-=f[7];f[0]-=f[4];f[1]-=f[5];f[2]-=f[6];f[3]-=f[7];return;}
        if (f.size()<64) {
            switch(f.size()) {
            case 16:reinsirk::detail::set_transform_small<false,true>(f.data(),16);return;
            case 32:reinsirk::detail::set_transform_small<false,true>(f.data(),32);return;
            default:return;
            }
        }
        if (__builtin_cpu_supports("avx2")) {
            reinsirk::detail::set_transform_simd<false,true>(f.data(), f.size());
            return;
        }
        }
    }
#endif
    for (std::size_t half = 1; half < f.size(); half *= 2) {
        for (std::size_t base = 0; base < f.size(); base += half * 2) {
            for (std::size_t j = 0; j < half; ++j) f[base + j] -= f[base + half + j];
        }
    }
}
#endif

#ifndef SIRKIT_SUBSET_ZETA_TRANSFORM_DEFINED
#define SIRKIT_SUBSET_ZETA_TRANSFORM_DEFINED
template <class T> void subset_zeta_transform(std::vector<T>& f) {
#ifndef SKIP_RUNTIME_CHECKS
    if (!f.empty() && !std::has_single_bit(f.size()))
        throw std::invalid_argument("subset_zeta_transform: length must be a power of two");
#endif
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T,bool> && sizeof(T)<=8) {
        if (f.size()>=8) {
        if(f.size()==8){f[1]+=f[0];f[3]+=f[2];f[5]+=f[4];f[7]+=f[6];f[2]+=f[0];f[3]+=f[1];f[6]+=f[4];f[7]+=f[5];f[4]+=f[0];f[5]+=f[1];f[6]+=f[2];f[7]+=f[3];return;}
        if (f.size()<64) {
            switch(f.size()) {
            case 16:reinsirk::detail::set_transform_small<true,false>(f.data(),16);return;
            case 32:reinsirk::detail::set_transform_small<true,false>(f.data(),32);return;
            default:return;
            }
        }
        if (__builtin_cpu_supports("avx2")) {
            reinsirk::detail::set_transform_simd<true,false>(f.data(), f.size());
            return;
        }
        }
    }
#endif
    for (std::size_t half = 1; half < f.size(); half *= 2) {
        for (std::size_t base = 0; base < f.size(); base += half * 2) {
            for (std::size_t j = 0; j < half; ++j) f[base + half + j] += f[base + j];
        }
    }
}
#endif

#ifndef SIRKIT_SUBSET_MOBIUS_TRANSFORM_DEFINED
#define SIRKIT_SUBSET_MOBIUS_TRANSFORM_DEFINED
template <class T> void subset_mobius_transform(std::vector<T>& f) {
#ifndef SKIP_RUNTIME_CHECKS
    if (!f.empty() && !std::has_single_bit(f.size()))
        throw std::invalid_argument("subset_mobius_transform: length must be a power of two");
#endif
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T,bool> && sizeof(T)<=8) {
        if (f.size()>=8) {
        if(f.size()==8){f[1]-=f[0];f[3]-=f[2];f[5]-=f[4];f[7]-=f[6];f[2]-=f[0];f[3]-=f[1];f[6]-=f[4];f[7]-=f[5];f[4]-=f[0];f[5]-=f[1];f[6]-=f[2];f[7]-=f[3];return;}
        if (f.size()<64) {
            switch(f.size()) {
            case 16:reinsirk::detail::set_transform_small<true,true>(f.data(),16);return;
            case 32:reinsirk::detail::set_transform_small<true,true>(f.data(),32);return;
            default:return;
            }
        }
        if (__builtin_cpu_supports("avx2")) {
            reinsirk::detail::set_transform_simd<true,true>(f.data(), f.size());
            return;
        }
        }
    }
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
#ifndef SIRKIT_BITWISE_AND_CONVOLUTION_DEFINED
#define SIRKIT_BITWISE_AND_CONVOLUTION_DEFINED
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

#ifndef SIRKIT_BITWISE_OR_CONVOLUTION_DEFINED
#define SIRKIT_BITWISE_OR_CONVOLUTION_DEFINED
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


