// SIRKIT_API_BEGIN
// [GCD／LCM畳み込み]
// gcd_convolution(f,g): h[k]=sum_{gcd(i,j)=k}f[i]g[j]。gcd(0,k)=k（gcd(0,0)=0）。
// lcm_convolution(f,g): h[k]=sum_{lcm(i,j)=k}f[i]g[j]。lcm(0,k)=0、結果k>=nは切り捨て。
// 両者は同じ長さnの入力（空も可）、O(1+n log log(n+2))時間・O(n+1)領域。値渡しでlvalue入力は保持。
// 各(f,g,sieve)版: sieve.max_value()>=n-1の素数表を再利用する。
// 各(f,g)版: 素数表も作る。n-1<=INT_MAX。T{}は零、環の加減乗算に対応（bool以外）。
// 符号付き整数の全途中結果はTの範囲内、符号なし整数は型の幅の剰余演算。
//
// gcd_square(f,sieve)/gcd_square(f): gcd_convolution(f,f)と同じ結果を返す。空も可、値渡し入力1本・変換2回。
// 時間O(1+n log log(n+2))・領域O(n+1)。表あり版はmax_value()>=n-1、表なし版はn-1<=INT_MAX。
//
// lcm_square(f,sieve)/lcm_square(f): lcm_convolution(f,f)と同じ結果を返す。空も可、値渡し入力1本・変換2回。
// 時間O(1+n log log(n+2))・領域O(n+1)。表あり版はmax_value()>=n-1、表なし版はn-1<=INT_MAX。
// SIRKIT_API_END
#ifndef SIRKIT_GCDCONV_DEFINED
#define SIRKIT_GCDCONV_DEFINED
template <class T> std::vector<T> gcd_convolution(std::vector<T> f, std::vector<T> g, const IntegerSieve& sieve) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_same_size(f.size(), g.size());
    reinsirk::detail::IntegerTransform::require_sieve(f.size(), sieve);
#endif
    return reinsirk::detail::IntegerConvolution::compute<true>(std::move(f), std::move(g), sieve.prime_list);
}
template <class T> std::vector<T> gcd_convolution(std::vector<T> f, std::vector<T> g) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_same_size(f.size(), g.size());
#endif
    std::vector<int> storage;
    const auto primes = reinsirk::detail::IntegerTransform::prime_view_for(f.size(), storage);
    return reinsirk::detail::IntegerConvolution::compute<true>(std::move(f), std::move(g), primes);
}
#endif
#ifndef SIRKIT_LCMCONV_DEFINED
#define SIRKIT_LCMCONV_DEFINED
template <class T> std::vector<T> lcm_convolution(std::vector<T> f, std::vector<T> g, const IntegerSieve& sieve) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_same_size(f.size(), g.size());
    reinsirk::detail::IntegerTransform::require_sieve(f.size(), sieve);
#endif
    return reinsirk::detail::IntegerConvolution::compute<false>(std::move(f), std::move(g), sieve.prime_list);
}
template <class T> std::vector<T> lcm_convolution(std::vector<T> f, std::vector<T> g) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_same_size(f.size(), g.size());
#endif
    std::vector<int> storage;
    const auto primes = reinsirk::detail::IntegerTransform::prime_view_for(f.size(), storage);
    return reinsirk::detail::IntegerConvolution::compute<false>(std::move(f), std::move(g), primes);
}
#endif

#ifndef SIRKIT_GCD_SQUARE_DEFINED
#define SIRKIT_GCD_SQUARE_DEFINED
template <class T> std::vector<T> gcd_square(std::vector<T> f, const IntegerSieve& sieve) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_sieve(f.size(), sieve);
#endif
    return reinsirk::detail::IntegerConvolution::square<true>(std::move(f), sieve.prime_list);
}
template <class T> std::vector<T> gcd_square(std::vector<T> f) {
    std::vector<int> storage;
    const auto primes = reinsirk::detail::IntegerTransform::prime_view_for(f.size(), storage);
    return reinsirk::detail::IntegerConvolution::square<true>(std::move(f), primes);
}
#endif
#ifndef SIRKIT_LCM_SQUARE_DEFINED
#define SIRKIT_LCM_SQUARE_DEFINED
template <class T> std::vector<T> lcm_square(std::vector<T> f, const IntegerSieve& sieve) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_sieve(f.size(), sieve);
#endif
    return reinsirk::detail::IntegerConvolution::square<false>(std::move(f), sieve.prime_list);
}
template <class T> std::vector<T> lcm_square(std::vector<T> f) {
    std::vector<int> storage;
    const auto primes = reinsirk::detail::IntegerTransform::prime_view_for(f.size(), storage);
    return reinsirk::detail::IntegerConvolution::square<false>(std::move(f), primes);
}
#endif
