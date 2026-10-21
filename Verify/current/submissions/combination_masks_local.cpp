// Verify: ローカル独立検査

// Library: Algorithm/next_combination.cpp
// SPDX-License-Identifier: CC0-1.0
// 参考: https://github.com/drken1215/algorithm/blob/master/Search/next_combination.cpp
#include <bit>
#include <concepts>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

#ifndef SIRKIT_NEXT_COMBINATION_DEFINED
#define SIRKIT_NEXT_COMBINATION_DEFINED
// next_combination(sub): 同じ1の個数で次に大きいマスク。0・型内に次がなければ0。O(1)。
// unsigned整数型に対応。既存int版は非負が前提で、正のintに収まらない次も0。
template <std::unsigned_integral UInt> requires (!std::same_as<UInt, bool>)
constexpr UInt next_combination(UInt sub) noexcept {
    if (sub == 0) return 0;
    const UInt low = static_cast<UInt>(sub & static_cast<UInt>(UInt{0} - sub));
    const UInt high = static_cast<UInt>(sub + low);
    if (high == 0) return 0;
    const UInt tail = static_cast<UInt>(static_cast<UInt>((sub ^ high) >> 2) >> std::countr_zero(sub));
    return static_cast<UInt>(high | tail);
}
inline constexpr int next_combination(int sub) {
#ifndef SKIP_RUNTIME_CHECKS
    if (sub < 0) throw std::out_of_range("next_combination: negative mask");
#endif
    const auto next = next_combination(static_cast<unsigned int>(sub));
    return next > static_cast<unsigned int>(std::numeric_limits<int>::max()) ? 0 : static_cast<int>(next);
}
#endif

#ifndef SIRKIT_COMBINATION_MASK_UTILITIES_DEFINED
#define SIRKIT_COMBINATION_MASK_UTILITIES_DEFINED
// first_combination<UInt=uint64_t>(k): 下位kビットが1の最小マスク。0<=k<=型幅。O(1)。
// next_combination(sub, n): 下位nビット内の次へsubを進める。次がなければfalseでsub不変。O(1)。
// for_each_combination<UInt=uint64_t>(n,k,visit): nビット内のk個の1を持つ全マスクを昇順にvisit(mask)へ渡す。k=0も1回。
// for_eachはC(n,k)回の呼出し・O(1)追加領域（visitの処理を除く）。入力検査は最初だけ行う。
// n,k,maskの検査はSKIPで省く。全幅のシフト・0除算を避ける処理は常に維持する。
namespace reinsirk::detail {
template <std::unsigned_integral UInt> requires (!std::same_as<UInt, bool>)
constexpr bool NextCombinationBounded(UInt& sub, int n) noexcept {
    constexpr int width = std::numeric_limits<UInt>::digits;
    const auto next = next_combination(sub);
    if (next == 0 || n <= 0 || (n < width && (next >> n) != 0)) return false;
    sub = next;
    return true;
}
} // namespace reinsirk::detail
template <std::unsigned_integral UInt = std::uint64_t> requires (!std::same_as<UInt, bool>)
constexpr UInt first_combination(int k) {
    constexpr int width = std::numeric_limits<UInt>::digits;
#ifndef SKIP_RUNTIME_CHECKS
    if (k < 0 || k > width) throw std::out_of_range("first_combination: invalid count");
#endif
    if (k <= 0) return 0;
    if (k >= width) return std::numeric_limits<UInt>::max();
    return static_cast<UInt>((UInt{1} << k) - UInt{1});
}
template <std::unsigned_integral UInt> requires (!std::same_as<UInt, bool>)
constexpr bool next_combination(UInt& sub, int n) {
#ifndef SKIP_RUNTIME_CHECKS
    constexpr int width = std::numeric_limits<UInt>::digits;
    if (n < 0 || n > width || (n < width && (sub >> n) != 0))
        throw std::out_of_range("next_combination: invalid width or mask");
#endif
    return reinsirk::detail::NextCombinationBounded(sub, n);
}
template <std::unsigned_integral UInt = std::uint64_t, class Visitor> requires (!std::same_as<UInt, bool>)
constexpr void for_each_combination(int n, int k, Visitor&& visit) {
#ifndef SKIP_RUNTIME_CHECKS
    constexpr int width = std::numeric_limits<UInt>::digits;
    if (n < 0 || n > width || k < 0 || k > n)
        throw std::out_of_range("for_each_combination: invalid width or count");
#endif
    if (n < 0 || k < 0 || k > n) return;
    UInt mask = first_combination<UInt>(k);
    do { visit(UInt{mask}); } while (reinsirk::detail::NextCombinationBounded(mask, n));
}
#endif


// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
int main(){
    auto check=[](bool ok){if(!ok)throw std::runtime_error("combination mask mismatch");};
    for(int n=0;n<=14;++n)for(int k=0;k<=n;++k){std::vector<std::uint64_t> ref,got;
        for(std::uint64_t i=0;i<(std::uint64_t{1}<<n);++i)if(std::popcount(i)==k)ref.push_back(i);
        for_each_combination(n,k,[&](std::uint64_t x){got.push_back(x);});check(got==ref);
        auto x=first_combination<std::uint64_t>(k);std::size_t i=0;do{check(x==ref[i++]);}while(next_combination(x,n));check(i==ref.size());
    }
    std::uint64_t last=std::uint64_t{1}<<63;check(next_combination(last)==0);check(!next_combination(last,64));
    check(first_combination<std::uint64_t>(64)==std::numeric_limits<std::uint64_t>::max());check(next_combination(7)==11);check(next_combination(0)==0);
    unsigned count=0;for_each_combination(64,64,[&](std::uint64_t x){++count;check(x==~std::uint64_t{0});});check(count==1);
    std::cout<<"OK\n";
}
