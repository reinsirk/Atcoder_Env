// Verify: ローカル独立検査

// Library: Algorithm/Radix_sort.cpp
// SPDX-License-Identifier: CC0-1.0
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/radix_sort.md
//
// API一覧
// [再利用作業領域]
// RadixSortWorkspace<T>(): boolを除く8/16/32/64bit整数用の再利用作業領域。
// sort(values): 符号付きも全域で安定昇順ソート。B=sizeof(T)としてO(Bn)、追加領域O(n)。
// sort(values,maximum): 同じ処理。全要素<=maximumが前提で、通常処理中に検査する。
// reserve(n): n要素分の作業領域を予約。capacity(): 現在の予約要素数。
// release(): 作業領域を解放する。sortはvaluesの容量・要素の保管先を変えない。
//
// [値の整列]
// RadixSort(values): 符号付きも全域で安定昇順ソート、O(sizeof(T)*n)時間・O(n)追加領域。
// RadixSort(values,maximum): 同じ処理。全要素<=maximumが前提（負の値・負のmaximumも可）。
// valuesの容量・要素の保管先は不変。型Tはboolを除く8/16/32/64bit整数。
//
// [添字の整列]
// RadixSortOrder(values): 昇順の元添字列を返す。同値は元の順、入力不変。
// boolを除く8/16/32/64bit整数。O(sizeof(T)*n)時間・O(n)追加領域。

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__x86_64__) || defined(__i386__)) && (defined(__GNUC__) || defined(__clang__))
#include <immintrin.h>
#endif

#ifndef SIRKIT_RADIX_SORT_DETAIL_DEFINED
#define SIRKIT_RADIX_SORT_DETAIL_DEFINED
namespace reinsirk::detail {
template <class T>
concept RadixInteger = std::integral<T> && !std::same_as<T, bool> &&
    !std::is_const_v<T> && !std::is_volatile_v<T> &&
    (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8) &&
    std::numeric_limits<std::make_unsigned_t<T>>::digits == sizeof(T) * 8;

template <RadixInteger T> struct RadixSortDetail {
    using Unsigned = std::make_unsigned_t<T>;
    static constexpr std::size_t bytes = sizeof(T);
    static constexpr std::size_t small_limit = 64;
    using Histogram = std::array<std::array<std::size_t, 256>, bytes>;

    static Unsigned key(T value) noexcept {
        Unsigned result = static_cast<Unsigned>(value);
        if constexpr (std::is_signed_v<T>) result ^= Unsigned{1} << (bytes * 8 - 1);
        return result;
    }
    template <bool HasMaximum> static void check(T value, [[maybe_unused]] T maximum) {
#ifndef SKIP_RUNTIME_CHECKS
        if constexpr (HasMaximum) {
            if (value > maximum) throw std::out_of_range("RadixSort: value exceeds maximum");
        }
#else
        (void)value;
#endif
    }
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__x86_64__) || defined(__i386__)) && (defined(__GNUC__) || defined(__clang__))
    // 順序と変動ビットを4要素ずつ求め、端数は標準の整数演算で処理する。
    template <bool HasMaximum>
    [[gnu::target("avx2")]]
    static Unsigned scan_avx2(const std::vector<T>& values, T maximum, bool& ordered, bool& reversed) {
        const auto first = std::bit_cast<std::int64_t>(values.front());
        const __m256i first_vector = _mm256_set1_epi64x(first);
        const __m256i sign = _mm256_set1_epi64x(std::numeric_limits<std::int64_t>::min());
        __m256i differences = _mm256_setzero_si256();
        __m256i greater = _mm256_setzero_si256(), less = _mm256_setzero_si256();
#ifndef SKIP_RUNTIME_CHECKS
        __m256i too_large = _mm256_setzero_si256();
        __m256i maximum_vector = _mm256_set1_epi64x(std::bit_cast<std::int64_t>(maximum));
        if constexpr (std::is_unsigned_v<T>) maximum_vector = _mm256_xor_si256(maximum_vector, sign);
#else
        (void)maximum;
#endif
        check<HasMaximum>(values.front(), maximum);
        std::size_t i = 1;
        for (; i + 4 <= values.size(); i += 4) {
            __m256i current = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(values.data() + i));
            __m256i previous = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(values.data() + i - 1));
            differences = _mm256_or_si256(differences, _mm256_xor_si256(current, first_vector));
            if constexpr (std::is_unsigned_v<T>) {
                current = _mm256_xor_si256(current, sign);
                previous = _mm256_xor_si256(previous, sign);
            }
            greater = _mm256_or_si256(greater, _mm256_cmpgt_epi64(previous, current));
            less = _mm256_or_si256(less, _mm256_cmpgt_epi64(current, previous));
#ifndef SKIP_RUNTIME_CHECKS
            if constexpr (HasMaximum)
                too_large = _mm256_or_si256(too_large, _mm256_cmpgt_epi64(current, maximum_vector));
#endif
        }
#ifndef SKIP_RUNTIME_CHECKS
        if constexpr (HasMaximum)
            if (!_mm256_testz_si256(too_large, too_large))
                throw std::out_of_range("RadixSort: value exceeds maximum");
#endif
        ordered = _mm256_testz_si256(greater, greater);
        reversed = _mm256_testz_si256(less, less);
        std::array<std::uint64_t, 4> lanes;
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(lanes.data()), differences);
        Unsigned different = lanes[0] | lanes[1] | lanes[2] | lanes[3];
        for (; i < values.size(); ++i) {
            check<HasMaximum>(values[i], maximum);
            ordered &= values[i - 1] <= values[i];
            reversed &= values[i - 1] >= values[i];
            different |= key(values[i]) ^ key(values.front());
        }
        return different;
    }
    static bool supports_avx2() noexcept {
        // コンパイラが初期化したCPU情報を参照する。
        return __builtin_cpu_supports("avx2");
    }
#endif
    template <bool HasMaximum>
    static Unsigned histogram(const std::vector<T>& values, Histogram& counts, T maximum, bool& ordered, bool& reversed) {
        if constexpr (bytes < 8) {
            counts = {};
            const Unsigned first = key(values.front());
            Unsigned different = 0;
            T previous = values.front();
            ordered = reversed = true;
            for (T value : values) {
                check<HasMaximum>(value, maximum);
                ordered &= previous <= value;
                reversed &= previous >= value;
                previous = value;
                Unsigned encoded = key(value);
                different |= encoded ^ first;
                if constexpr (bytes == 4) {
                    ++counts[0][encoded & Unsigned{255}];
                    ++counts[1][(encoded >> 8) & Unsigned{255}];
                    ++counts[2][(encoded >> 16) & Unsigned{255}];
                    ++counts[3][(encoded >> 24) & Unsigned{255}];
                } else {
                    for (std::size_t byte = 0; byte < bytes; ++byte) {
                        ++counts[byte][encoded & Unsigned{255}];
                        if constexpr (bytes > 1) encoded >>= 8;
                    }
                }
            }
            return different;
        } else {
            const Unsigned first = key(values.front());
            Unsigned different = 0;
#if !defined(SIRKIT_DISABLE_SIMD) && (defined(__x86_64__) || defined(__i386__)) && (defined(__GNUC__) || defined(__clang__))
            if (values.size() >= 128 && supports_avx2())
                different = scan_avx2<HasMaximum>(values, maximum, ordered, reversed);
            else
#endif
            {
                T previous = values.front();
                ordered = reversed = true;
                for (T value : values) {
                    check<HasMaximum>(value, maximum);
                    ordered &= previous <= value;
                    reversed &= previous >= value;
                    previous = value;
                    different |= key(value) ^ first;
                }
            }
            // 整列済みなら集計しない。散布する上位バイト数は差分から決める。
            if (ordered || reversed) return different;
            counts = {};
            if constexpr (bytes > 4) {
                const std::size_t active = (std::bit_width(different) + 7) / 8;
                if (active <= 4) {
                    for (T value : values) {
                        const Unsigned encoded = key(value);
                        ++counts[0][encoded & Unsigned{255}];
                        if (active > 1) ++counts[1][(encoded >> 8) & Unsigned{255}];
                        if (active > 2) ++counts[2][(encoded >> 16) & Unsigned{255}];
                        if (active > 3) ++counts[3][(encoded >> 24) & Unsigned{255}];
                    }
                    return different;
                }
            }
            for (T value : values) {
                Unsigned encoded = key(value);
                for (std::size_t byte = 0; byte < bytes; ++byte) {
                    ++counts[byte][encoded & Unsigned{255}];
                    if constexpr (bytes > 1) encoded >>= 8;
                }
            }
            return different;
        }
    }
    static void prefix(std::array<std::size_t, 256>& counts) noexcept {
        std::size_t offset = 0;
        for (auto& count : counts) {
            const auto current = count;
            count = offset;
            offset += current;
        }
    }
    template <bool HasMaximum>
    static void sort(std::vector<T>& values, std::vector<T>& buffer, T maximum) {
        const std::size_t n = values.size();
        if (n <= small_limit) {
            if (n) check<HasMaximum>(values[0], maximum);
            for (std::size_t i = 1; i < n; ++i) {
                const T value = values[i];
                check<HasMaximum>(value, maximum);
                std::size_t j = i;
                for (; j && value < values[j - 1]; --j) values[j] = values[j - 1];
                values[j] = value;
            }
            return;
        }
        Histogram counts;
        bool ordered, reversed;
        const Unsigned different = histogram<HasMaximum>(values, counts, maximum, ordered, reversed);
        if (ordered) return;
        if (reversed) {
            std::reverse(values.begin(), values.end());
            return;
        }
        buffer.resize(n);
        T* source = values.data();
        T* destination = buffer.data();
        for (std::size_t byte = 0; byte < bytes; ++byte) {
            const std::size_t shift = byte * 8;
            if (((different >> shift) & Unsigned{255}) == 0) continue;
            auto& offsets = counts[byte];
            prefix(offsets);
            for (std::size_t i = 0; i < n; ++i) {
                const T value = source[i];
                const auto bucket = static_cast<std::size_t>((key(value) >> shift) & Unsigned{255});
                destination[offsets[bucket]++] = value;
            }
            std::swap(source, destination);
        }
        if (source != values.data()) std::copy_n(source, n, values.data());
    }
};
} // namespace reinsirk::detail
#endif

#ifndef SIRKIT_RADIX_SORT_WORKSPACE_DEFINED
#define SIRKIT_RADIX_SORT_WORKSPACE_DEFINED
template <reinsirk::detail::RadixInteger T> class RadixSortWorkspace {
    std::vector<T> buffer_;
public:
    void sort(std::vector<T>& values) {
        reinsirk::detail::RadixSortDetail<T>::template sort<false>(values, buffer_, T{});
    }
    void sort(std::vector<T>& values, T maximum) {
        reinsirk::detail::RadixSortDetail<T>::template sort<true>(values, buffer_, maximum);
    }
    void reserve(std::size_t count) { buffer_.reserve(count); }
    std::size_t capacity() const noexcept { return buffer_.capacity(); }
    void release() noexcept { std::vector<T>().swap(buffer_); }
};
#endif

#ifndef SIRKIT_RADIX_SORT_DEFINED
#define SIRKIT_RADIX_SORT_DEFINED
// 小配列は確保せず挿入ソート。整列済みは変更せず、一定のbyteは散布を省く。
template <reinsirk::detail::RadixInteger T> void RadixSort(std::vector<T>& values) {
    RadixSortWorkspace<T> workspace;
    workspace.sort(values);
}
template <reinsirk::detail::RadixInteger T> void RadixSort(std::vector<T>& values, T maximum) {
    RadixSortWorkspace<T> workspace;
    workspace.sort(values, maximum);
}
#endif

#ifndef SIRKIT_RADIX_SORT_ORDER_DEFINED
#define SIRKIT_RADIX_SORT_ORDER_DEFINED
template <reinsirk::detail::RadixInteger T> std::vector<std::size_t> RadixSortOrder(const std::vector<T>& values) {
    using Detail = reinsirk::detail::RadixSortDetail<T>;
    using Unsigned = typename Detail::Unsigned;
    const std::size_t n = values.size();
    std::vector<std::size_t> order(n);
    std::iota(order.begin(), order.end(), std::size_t{0});
    if (n <= Detail::small_limit) {
        for (std::size_t i = 1; i < n; ++i) {
            const std::size_t index = order[i];
            std::size_t j = i;
            for (; j && values[index] < values[order[j - 1]]; --j) order[j] = order[j - 1];
            order[j] = index;
        }
        return order;
    }
    typename Detail::Histogram counts;
    bool ordered, reversed;
    const Unsigned different = Detail::template histogram<false>(values, counts, T{}, ordered, reversed);
    if (ordered) return order;
    if (reversed) {
        // 同じ値の区間内は元の添字順を保つ。
        std::size_t output = 0, end = n;
        while (end) {
            std::size_t begin = end - 1;
            while (begin && values[begin - 1] == values[end - 1]) --begin;
            for (std::size_t i = begin; i < end; ++i) order[output++] = i;
            end = begin;
        }
        return order;
    }
    std::vector<std::size_t> buffer(n);
    for (std::size_t byte = 0; byte < Detail::bytes; ++byte) {
        const std::size_t shift = byte * 8;
        if (((different >> shift) & Unsigned{255}) == 0) continue;
        auto& offsets = counts[byte];
        Detail::prefix(offsets);
        for (std::size_t index : order) {
            const auto bucket = static_cast<std::size_t>((Detail::key(values[index]) >> shift) & Unsigned{255});
            buffer[offsets[bucket]++] = index;
        }
        order.swap(buffer);
    }
    return order;
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
#include <limits>
#include <random>
#include <string>
#include <utility>
#include <vector>

template<class T> void verify_radix(std::mt19937_64& generator) {
    RadixSortWorkspace<T> workspace;
    for (int n : {0, 1, 2, 63, 64, 65, 1000}) {
        std::vector<T> values(n);
        for (auto& x : values) x = static_cast<T>(generator());
        if (n >= 2) { values[0] = std::numeric_limits<T>::min(); values[1] = std::numeric_limits<T>::max(); }
        auto expected = values; std::stable_sort(expected.begin(), expected.end());
        auto actual = values; workspace.sort(actual); assert(actual == expected);
        actual = values; RadixSort(actual); assert(actual == expected);
        actual = values; workspace.sort(actual, std::numeric_limits<T>::max()); assert(actual == expected);
        auto order = RadixSortOrder(values);
        std::vector<std::size_t> reference(values.size()); std::iota(reference.begin(), reference.end(), 0);
        std::stable_sort(reference.begin(), reference.end(), [&](auto a, auto b) { return values[a] < values[b]; });
        assert(order == reference);
        std::reverse(expected.begin(), expected.end()); workspace.sort(expected); assert(expected == actual);
    }
    workspace.release(); assert(workspace.capacity() == 0);
}
int main() {
    std::mt19937_64 generator(91234);
    verify_radix<std::int8_t>(generator); verify_radix<std::uint8_t>(generator);
    verify_radix<std::int16_t>(generator); verify_radix<std::uint16_t>(generator);
    verify_radix<std::int32_t>(generator); verify_radix<std::uint32_t>(generator);
    verify_radix<std::int64_t>(generator); verify_radix<std::uint64_t>(generator);
    std::cout << "OK\n";
}
