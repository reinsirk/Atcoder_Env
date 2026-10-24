#ifndef SIRKIT_GEO_EPSILON_DEFINED
#define SIRKIT_GEO_EPSILON_DEFINED
namespace geo {
// SIRKIT_API_BEGIN
// [geo::Epsilon]
// Epsilon{abs,rel}: 非負有限の絶対・相対許容幅（既定は各1e-12L）。
// tolerance(scale): abs + rel * abs(scale) を返す。O(1)。
// SIRKIT_API_END
struct Epsilon {
    long double abs = 1e-12L;
    long double rel = 1e-12L;
    constexpr long double tolerance(long double scale) const {
        return abs + rel * std::abs(scale);
    }
};
}
#endif
