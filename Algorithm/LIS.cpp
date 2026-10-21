// SPDX-License-Identifier: CC0-1.0
// 参考: https://zenn.dev/reputeless/books/standard-cpp-for-competitive-programming/viewer/lis
#include <algorithm>
#include <cstddef>
#include <limits>
#include <vector>

#ifndef REINSIRK_LIS_LENGTHS_DEFINED
#define REINSIRK_LIS_LENGTHS_DEFINED
// LIS<Strict>(v): 各接頭辞のLIS長を返す。trueは狭義、falseは広義。空入力は空。
// LISLength<Strict=true>(v): LIS長だけを返す。入力は変更せず、比較はoperator<を使う。
// 時間O(N log(L+1))、追加領域はLISがO(N+L)、LISLengthがO(L)。整列済みの増加列はO(N)。
namespace reinsirk::detail {
template <bool Strict, bool Prefix, class T>
auto LISLengths(const std::vector<T>& v) {
    std::vector<T> tails;
    tails.reserve(std::min(v.size(), std::size_t{64}));
    std::vector<std::size_t> lengths;
    if constexpr (Prefix) lengths.resize(v.size());
    std::size_t i = 0;
    for (const auto& value : v) {
        const bool extend = tails.empty() || (Strict ? tails.back() < value : !(value < tails.back()));
        if (extend) tails.push_back(value);
        else {
            if constexpr (Strict) *std::lower_bound(tails.begin(), tails.end(), value) = value;
            else *std::upper_bound(tails.begin(), tails.end(), value) = value;
        }
        if constexpr (Prefix) lengths[i++] = tails.size();
    }
    if constexpr (Prefix) return lengths;
    else return tails.size();
}
} // namespace reinsirk::detail
template <bool Strict, class T>
std::vector<std::size_t> LIS(const std::vector<T>& v) {
    return reinsirk::detail::LISLengths<Strict, true>(v);
}
template <bool Strict = true, class T>
std::size_t LISLength(const std::vector<T>& v) {
    return reinsirk::detail::LISLengths<Strict, false>(v);
}
#endif

#ifndef REINSIRK_LIS_RESTORE_DEFINED
#define REINSIRK_LIS_RESTORE_DEFINED
// LISIndices<Strict=true>(v): LISを1つ復元し、元配列の添字を昇順で返す。同長の解の選択は任意。
// LISSequence<Strict=true>(v): LISを1つ値の列としてコピーする。空なら空。いずれもO(N log(L+1))時間・O(N)追加領域。
template <bool Strict = true, class T>
std::vector<std::size_t> LISIndices(const std::vector<T>& v) {
    constexpr auto none = std::numeric_limits<std::size_t>::max();
    std::vector<std::size_t> tails, previous(v.size(), none);
    tails.reserve(std::min(v.size(), std::size_t{64}));
    for (std::size_t i = 0; i < v.size(); ++i) {
        std::size_t lo = 0, hi = tails.size();
        if (!tails.empty() && (Strict ? v[tails.back()] < v[i] : !(v[i] < v[tails.back()]))) lo = hi;
        else while (lo < hi) {
            const auto mid = lo + (hi - lo) / 2;
            const bool before = Strict ? v[tails[mid]] < v[i] : !(v[i] < v[tails[mid]]);
            if (before) lo = mid + 1;
            else hi = mid;
        }
        if (lo != 0) previous[i] = tails[lo - 1];
        if (lo == tails.size()) tails.push_back(i);
        else tails[lo] = i;
    }
    std::vector<std::size_t> result(tails.size());
    if (!tails.empty()) {
        auto p = tails.back();
        for (auto i = result.size(); i != 0; --i) {
            result[i - 1] = p;
            p = previous[p];
        }
    }
    return result;
}
template <bool Strict = true, class T>
std::vector<T> LISSequence(const std::vector<T>& v) {
    const auto indices = LISIndices<Strict>(v);
    std::vector<T> result;
    result.reserve(indices.size());
    for (auto i : indices) result.push_back(v[i]);
    return result;
}
#endif
