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


