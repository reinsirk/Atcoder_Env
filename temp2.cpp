// Library Codes are in: https://github.com/reinsirk/sirkit-cpp-library
// ドキュメント: https://github.com/reinsirk/sirkit-cpp-library/blob/main/Document/template.md
//
// API一覧
// [共有型・定数]
// Graph: 無重み隣接リスト。Edge{to,w}/WGraph: 行先int・重みint64_tの隣接リスト。
// INF: int64_t{1}<<60。型・定数はライブラリの共通ガードで共有する。
// [補助関数]
// chmin(a, b): b < a なら a を更新して true を返す。比較・代入各1回以下。
// chmax(a, b): a < b なら a を更新して true を返す。比較・代入各1回以下。
// pow64i(x, n): x^n mod 2^64 を O(log(n + 1)) で返す。0^0 は1。
// is_mul_overflow(a, b): a*b が符号付き64ビット整数の範囲外なら true。O(1)。

#include <bits/stdc++.h>
// #include <atcoder/all>

using namespace std;
// using namespace atcoder;

#define SKIP_RUNTIME_CHECKS

#ifndef SIRKIT_INF_DEFINED
#define SIRKIT_INF_DEFINED
inline constexpr std::int64_t INF = std::int64_t{1} << 60;
#endif

#ifndef SIRKIT_UNWEIGHTED_GRAPH_DEFINED
#define SIRKIT_UNWEIGHTED_GRAPH_DEFINED
using Graph = std::vector<std::vector<int>>;
#endif

#ifndef SIRKIT_GRAPH_TYPES_DEFINED
#define SIRKIT_GRAPH_TYPES_DEFINED
struct Edge {
    int to;
    std::int64_t w;
    Edge(int destination, std::int64_t weight) : to(destination), w(weight) {}
};
using WGraph = std::vector<std::vector<Edge>>;
#endif

#ifndef SIRKIT_CHMIN_DEFINED
#define SIRKIT_CHMIN_DEFINED
#define SIRKIT_TEMP_DEFINE_CHMIN
template <class T> constexpr bool chmin(T& a, T b);
#endif

#ifndef SIRKIT_CHMAX_DEFINED
#define SIRKIT_CHMAX_DEFINED
#define SIRKIT_TEMP_DEFINE_CHMAX
template <class T> constexpr bool chmax(T& a, T b);
#endif

#ifndef SIRKIT_POW64I_DEFINED
#define SIRKIT_POW64I_DEFINED
#define SIRKIT_TEMP_DEFINE_POW64I
constexpr std::uint64_t pow64i(std::uint64_t x, std::uint64_t n);
#endif

#ifndef SIRKIT_IS_MUL_OVERFLOW_DEFINED
#define SIRKIT_IS_MUL_OVERFLOW_DEFINED
#define SIRKIT_TEMP_DEFINE_IS_MUL_OVERFLOW
constexpr bool is_mul_overflow(std::int64_t a, std::int64_t b);
#endif
// cout << std::fixed << std::setprecision(15);

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    return 0;
}

#ifdef SIRKIT_TEMP_DEFINE_CHMIN
#undef SIRKIT_TEMP_DEFINE_CHMIN
template <class T>
constexpr bool chmin(T& a, T b) {
    if (b < a) {
        a = b;
        return true;
    }
    return false;
}
#endif

#ifdef SIRKIT_TEMP_DEFINE_CHMAX
#undef SIRKIT_TEMP_DEFINE_CHMAX
template <class T>
constexpr bool chmax(T& a, T b) {
    if (a < b) {
        a = b;
        return true;
    }
    return false;
}
#endif

#ifdef SIRKIT_TEMP_DEFINE_POW64I
#undef SIRKIT_TEMP_DEFINE_POW64I
constexpr std::uint64_t pow64i(std::uint64_t x, std::uint64_t n) {
    std::uint64_t result = 1;
    while (n != 0) {
        if ((n & 1) != 0) result *= x;
        n >>= 1;
        if (n != 0) x *= x;
    }
    return result;
}
#endif

#ifdef SIRKIT_TEMP_DEFINE_IS_MUL_OVERFLOW
#undef SIRKIT_TEMP_DEFINE_IS_MUL_OVERFLOW
constexpr bool is_mul_overflow(std::int64_t a, std::int64_t b) {
#if (defined(__GNUC__) || defined(__clang__)) && !defined(SIRKIT_FORCE_STANDARD_MUL_OVERFLOW)
    std::int64_t product;
    return __builtin_mul_overflow(a, b, &product);
#else
    // 最小負数の絶対値も符号なし演算で安全に求める。
    const std::uint64_t ua = a < 0 ? std::uint64_t{0} - static_cast<std::uint64_t>(a)
                                   : static_cast<std::uint64_t>(a);
    const std::uint64_t ub = b < 0 ? std::uint64_t{0} - static_cast<std::uint64_t>(b)
                                   : static_cast<std::uint64_t>(b);
    const std::uint64_t limit = (a < 0) != (b < 0)
        ? std::uint64_t{1} << 63
        : static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    return ub != 0 && ua > limit / ub;
#endif
}
#endif
