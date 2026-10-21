// 正の添字の4変換。f[0]は不変、空配列も可。時間O(n log log n)。
// multiple_zeta_transform(f): f[d]=sum_{d|k} f[k]へ更新する。
// multiple_mobius_transform(f): 倍数ゼータ変換を元に戻す。
// divisor_zeta_transform(f): f[k]=sum_{d|k} f[d]へ更新する。
// divisor_mobius_transform(f): 約数ゼータ変換を元に戻す。
// 各(f,sieve)版: sieve.max_value()>=f.size()-1の素数表を再利用し、追加領域O(1)。
// 各(f)版: 素数表も作る。f.size()-1<=INT_MAX、追加領域O(n)。
#ifndef REINSIRK_MULTIPLE_ZETA_DEFINED
#define REINSIRK_MULTIPLE_ZETA_DEFINED
template <class T> void multiple_zeta_transform(std::vector<T>& f, const IntegerSieve& sieve) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_sieve(f.size(), sieve);
#endif
    reinsirk::detail::IntegerTransform::propagate<false, false>(f, sieve.prime_list);
}
template <class T> void multiple_zeta_transform(std::vector<T>& f) {
    std::vector<int> storage;
    const auto primes = reinsirk::detail::IntegerTransform::prime_view_for(f.size(), storage);
    reinsirk::detail::IntegerTransform::propagate<false, false>(f, primes);
}
#endif
#ifndef REINSIRK_MULTIPLE_MOEBIUS_DEFINED
#define REINSIRK_MULTIPLE_MOEBIUS_DEFINED
template <class T> void multiple_mobius_transform(std::vector<T>& f, const IntegerSieve& sieve) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_sieve(f.size(), sieve);
#endif
    reinsirk::detail::IntegerTransform::propagate<false, true>(f, sieve.prime_list);
}
template <class T> void multiple_mobius_transform(std::vector<T>& f) {
    std::vector<int> storage;
    const auto primes = reinsirk::detail::IntegerTransform::prime_view_for(f.size(), storage);
    reinsirk::detail::IntegerTransform::propagate<false, true>(f, primes);
}
#endif
#ifndef REINSIRK_DIVISOR_ZETA_DEFINED
#define REINSIRK_DIVISOR_ZETA_DEFINED
template <class T> void divisor_zeta_transform(std::vector<T>& f, const IntegerSieve& sieve) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_sieve(f.size(), sieve);
#endif
    reinsirk::detail::IntegerTransform::propagate<true, false>(f, sieve.prime_list);
}
template <class T> void divisor_zeta_transform(std::vector<T>& f) {
    std::vector<int> storage;
    const auto primes = reinsirk::detail::IntegerTransform::prime_view_for(f.size(), storage);
    reinsirk::detail::IntegerTransform::propagate<true, false>(f, primes);
}
#endif
#ifndef REINSIRK_DIVISOR_MOEBIUS_DEFINED
#define REINSIRK_DIVISOR_MOEBIUS_DEFINED
template <class T> void divisor_mobius_transform(std::vector<T>& f, const IntegerSieve& sieve) {
#ifndef SKIP_RUNTIME_CHECKS
    reinsirk::detail::IntegerTransform::require_sieve(f.size(), sieve);
#endif
    reinsirk::detail::IntegerTransform::propagate<true, true>(f, sieve.prime_list);
}
template <class T> void divisor_mobius_transform(std::vector<T>& f) {
    std::vector<int> storage;
    const auto primes = reinsirk::detail::IntegerTransform::prime_view_for(f.size(), storage);
    reinsirk::detail::IntegerTransform::propagate<true, true>(f, primes);
}
#endif

