// SPDX-License-Identifier: CC0-1.0
// Original Source: https://zenn.dev/reputeless/books/standard-cpp-for-competitive-programming/viewer/lis
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/lis.md

// API一覧
// [長さ・接頭辞]
// 比較はoperator<の狭義弱順序。浮動NaNは検査時に拒否、±infは許可。
// LIS/Length/EndingLengthsはTのコピー構築・代入、Sequenceはコピー構築が必要。
// LIS<Strict>(v): 各接頭辞のLIS長を返す。trueは狭義、falseは広義。空入力は空。
// LISLength<Strict=true>(v): LIS長だけを返す。入力は変更せず、比較はoperator<を使う。
// 時間O(N log(L+1))、追加領域はLISがO(N+L)、LISLengthがO(L)。整列済みの増加列はO(N)。
//
// [復元]
// LISIndicesはconst Tの比較だけを使い、コピー不能・非default構築の型も扱う。
// LISIndices<Strict=true>(v): LISを1つ復元し、元配列の添字を昇順で返す。同長の解の選択は任意。
// LISSequence<Strict=true>(v): LISを1つ値の列としてコピーする。空なら空。いずれもO(N log(L+1))時間・O(N)追加領域。
//
// [各末尾の長さ]
// LISEndingLengths<Strct=true>(v): 各要素を末尾とする最長長。狭義/広義、空なら空、O(N log(L+1))時間・O(N+L)領域。
// https://miscalc.hatenablog.com/entry/2024/07/25/212618

#include <algorithm>
#include <type_traits>
#include <stdexcept>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

#ifndef SKIP_RUNTIME_CHECKS
#ifndef SIRKIT_LIS_INPUT_CHECK_DEFINED
#define SIRKIT_LIS_INPUT_CHECK_DEFINED
namespace reinsirk::detail {
template <class T>
void check_lis_value(const T& value) {
    if constexpr (std::is_floating_point_v<T>)
        if (std::isnan(value)) throw std::invalid_argument("LIS: NaN input");
}
} // namespace reinsirk::detail
#endif
#endif

#ifndef SIRKIT_LIS_LENGTHS_DEFINED
#define SIRKIT_LIS_LENGTHS_DEFINED
namespace reinsirk::detail {
template <bool Strict, bool Prefix, class T>
auto LISLengths(const std::vector<T>& v) {
    std::vector<T> tails;
    tails.reserve(std::min(v.size(), std::size_t{64}));
    std::vector<std::size_t> lengths;
    if constexpr (Prefix) lengths.resize(v.size());
    std::size_t i = 0;
    for (const auto& value : v) {
#ifndef SKIP_RUNTIME_CHECKS
        reinsirk::detail::check_lis_value(value);
#endif
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

#ifndef SIRKIT_LIS_RESTORE_DEFINED
#define SIRKIT_LIS_RESTORE_DEFINED
template <bool Strict = true, class T>
std::vector<std::size_t> LISIndices(const std::vector<T>& v) {
    if (v.empty()) return {};
    std::vector<std::size_t> tails;
    auto previous = std::make_unique_for_overwrite<std::size_t[]>(v.size());
    tails.reserve(std::min(v.size(), std::size_t{64}));
    for (std::size_t i = 0; i < v.size(); ++i) {
#ifndef SKIP_RUNTIME_CHECKS
        reinsirk::detail::check_lis_value(v[i]);
#endif
        std::size_t lo = 0, hi = tails.size();
        if (tails.empty() || (Strict ? v[tails.back()] < v[i] : !(v[i] < v[tails.back()]))) lo = hi;
        else if (!tails.empty() && (Strict ? !(v[tails.front()] < v[i]) : v[i] < v[tails.front()])) hi = 0;
        else {
            lo = 1;
            // 末尾は延長判定済み。間の候補だけを比較し、必要なら末尾へ置く。
            std::size_t count = hi - 2;
            while (count != 0) {
                const auto half = count / 2;
                const auto mid = lo + half;
                const bool before = Strict ? v[tails[mid]] < v[i] : !(v[i] < v[tails[mid]]);
                lo = before ? mid + 1 : lo;
                count = before ? count - half - 1 : half;
            }
        }
        if (lo != 0) previous[i] = tails[lo - 1];
        if (lo == tails.size()) tails.push_back(i);
        else tails[lo] = i;
    }
    std::vector<std::size_t> result(tails.size());
    if (!tails.empty()) {
        auto p = tails.back();
        for (auto i = result.size(); i > 1; --i) {
            result[i - 1] = p;
            p = previous[p];
        }
        result[0] = p;
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

#ifndef SIRKIT_LIS_ENDING_LENGTHS_DEFINED
#define SIRKIT_LIS_ENDING_LENGTHS_DEFINED
template <bool Strict = true, class T>
std::vector<std::size_t> LISEndingLengths(const std::vector<T>& v) {
    std::vector<T> tails;
    tails.reserve(std::min(v.size(), std::size_t{64}));
    std::vector<std::size_t> result;
    result.reserve(v.size());
    for (const auto& value : v) {
#ifndef SKIP_RUNTIME_CHECKS
        reinsirk::detail::check_lis_value(value);
#endif
        if (tails.empty() || (Strict ? tails.back() < value : !(value < tails.back()))) {
            tails.push_back(value);
            result.push_back(tails.size());
        } else {
            auto it = Strict ? std::lower_bound(tails.begin(), tails.end(), value)
                             : std::upper_bound(tails.begin(), tails.end(), value);
            const auto length = static_cast<std::size_t>(it - tails.begin()) + 1;
            *it = value;
            result.push_back(length);
        }
    }
    return result;
}
#endif
