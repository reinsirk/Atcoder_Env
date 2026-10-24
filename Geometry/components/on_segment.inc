#ifndef SIRKIT_GEO_ON_SEGMENT_DEFINED
#define SIRKIT_GEO_ON_SEGMENT_DEFINED
namespace geo {
// SIRKIT_API_BEGIN
// [geo::on_segment]
// on_segment(p,s,eps): 閉線分上（端点を含む）なら true。整数では厳密。O(1)。
// SIRKIT_API_END
template<class T, class C> bool on_segment(const Point<T, C>& p, const Segment<T, C>& s, Epsilon eps = {}) {
    if (orientation(s.a, s.b, p, eps) != 0) return false;
    C x = (C(p.x) - C(s.a.x)) * (C(p.x) - C(s.b.x));
    C y = (C(p.y) - C(s.a.y)) * (C(p.y) - C(s.b.y));
    if constexpr (std::is_integral_v<T>) return x + y <= 0;
    else return predicate_sign<T, C>(x + y, std::abs(x) + std::abs(y), eps) <= 0;
}
}
#endif
