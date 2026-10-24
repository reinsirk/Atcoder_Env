// SPDX-License-Identifier: CC0-1.0
// 参考: https://nyaannyaan.github.io/library/set-function/zeta-mobius-transform.hpp.html
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/zeta_mobius.md
//
// API一覧
// [集合のゼータ／メビウス変換]
// 集合4変換: 空配列または長さn=2^kを直接更新、O(1+n log(n+1))時間・O(1)追加領域。
// superset_zeta_transform(f): f[S]を上位集合の総和へ更新、Tの+=が必要。
// superset_mobius_transform(f): 上位集合ゼータ変換の逆、Tの-=が必要。
// subset_zeta_transform(f): f[S]を部分集合の総和へ更新、Tの+=が必要。
// subset_mobius_transform(f): 部分集合ゼータ変換の逆、Tの-=が必要。
// Tはbool以外の加法型。符号付き整数の全途中結果は型の範囲内、符号なしは型の幅の剰余。値演算送出時は部分更新。

#include <bit>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef SIRKIT_SET_TRANSFORM_SCALAR_DETAIL_DEFINED
#define SIRKIT_SET_TRANSFORM_SCALAR_DETAIL_DEFINED
namespace reinsirk::detail {
#if defined(__GNUC__) && !defined(__clang__)
template <bool Subtract, class T>
[[gnu::always_inline]] inline void set_transform_eight(T* out, T* in) {
    if constexpr (Subtract) out[0] -= in[0]; else out[0] += in[0];
    if constexpr (Subtract) out[1] -= in[1]; else out[1] += in[1];
    if constexpr (Subtract) out[2] -= in[2]; else out[2] += in[2];
    if constexpr (Subtract) out[3] -= in[3]; else out[3] += in[3];
    if constexpr (Subtract) out[4] -= in[4]; else out[4] += in[4];
    if constexpr (Subtract) out[5] -= in[5]; else out[5] += in[5];
    if constexpr (Subtract) out[6] -= in[6]; else out[6] += in[6];
    if constexpr (Subtract) out[7] -= in[7]; else out[7] += in[7];
}
#endif
template <bool Subset, bool Subtract, class T>
inline void set_transform_scalar(std::vector<T>& f) {
    const std::size_t n = f.size();
    if (n >= 8) {
        for (std::size_t base = 0; base < n; base += 8) {
            auto add = [&](std::size_t a, std::size_t b) {
                if constexpr (Subtract) f[base + a] -= f[base + b];
                else f[base + a] += f[base + b];
            };
            if constexpr (Subset) {
                add(1, 0); add(3, 2); add(5, 4); add(7, 6);
                add(2, 0); add(3, 1); add(6, 4); add(7, 5);
                add(4, 0); add(5, 1); add(6, 2); add(7, 3);
            } else {
                add(0, 1); add(2, 3); add(4, 5); add(6, 7);
                add(0, 2); add(1, 3); add(4, 6); add(5, 7);
                add(0, 4); add(1, 5); add(2, 6); add(3, 7);
            }
        }
    }
    std::size_t half = n >= 8 ? 8 : 1;
#if defined(__GNUC__) && !defined(__clang__)
    if constexpr (Subset && std::is_trivially_copyable_v<T> && sizeof(T) <= 8) {
        // 依存する段の順序を保って2段をまとめる。
        for (; half >= 8 && half < n / 2; half *= 4) {
            for (std::size_t base = 0; base < n; base += half * 4) {
                auto* a = f.data() + base;
                auto* b = a + half;
                auto* c = b + half;
                auto* d = c + half;
                for (std::size_t j = 0; j < half; j += 8) {
                    set_transform_eight<Subtract>(b+j,a+j);
                    set_transform_eight<Subtract>(d+j,c+j);
                    set_transform_eight<Subtract>(c+j,a+j);
                    set_transform_eight<Subtract>(d+j,b+j);
                }
            }
        }
    }
#endif
    for (; half < n; half *= 2) {
        for (std::size_t base = 0; base < n; base += half * 2) {
#if defined(__GNUC__) && !defined(__clang__)
            // 小さな単純型は8要素ずつ展開し、GCCの自動ベクトル化を促す。
            if constexpr (std::is_trivially_copyable_v<T> && sizeof(T) <= 8) {
                auto* out = f.data() + base + (Subset ? half : 0);
                auto* in = f.data() + base + (Subset ? 0 : half);
                std::size_t j = 0;
                for (; half - j >= 8; j += 8) {
                    if constexpr (Subtract) out[j] -= in[j];
                    else out[j] += in[j];
                    if constexpr (Subtract) out[j+1] -= in[j+1];
                    else out[j+1] += in[j+1];
                    if constexpr (Subtract) out[j+2] -= in[j+2];
                    else out[j+2] += in[j+2];
                    if constexpr (Subtract) out[j+3] -= in[j+3];
                    else out[j+3] += in[j+3];
                    if constexpr (Subtract) out[j+4] -= in[j+4];
                    else out[j+4] += in[j+4];
                    if constexpr (Subtract) out[j+5] -= in[j+5];
                    else out[j+5] += in[j+5];
                    if constexpr (Subtract) out[j+6] -= in[j+6];
                    else out[j+6] += in[j+6];
                    if constexpr (Subtract) out[j+7] -= in[j+7];
                    else out[j+7] += in[j+7];
                }
                for (; j < half; ++j) {
                    if constexpr (Subtract) out[j] -= in[j];
                    else out[j] += in[j];
                }
            } else
#endif
            {
                for (std::size_t j = 0; j < half; ++j) {
                    auto& out = f[base + j + (Subset ? half : 0)];
                    auto& in = f[base + j + (Subset ? 0 : half)];
                    if constexpr (Subtract) out -= in;
                    else out += in;
                }
            }
        }
    }
}
}
#endif

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
    reinsirk::detail::set_transform_scalar<false, false>(f);
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
    reinsirk::detail::set_transform_scalar<false, true>(f);
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
    reinsirk::detail::set_transform_scalar<true, false>(f);
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
    reinsirk::detail::set_transform_scalar<true, true>(f);
}
#endif

