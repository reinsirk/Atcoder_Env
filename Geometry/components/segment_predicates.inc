#ifndef SIRKIT_GEO_SEGMENT_PREDICATES_DEFINED
#define SIRKIT_GEO_SEGMENT_PREDICATES_DEFINED
namespace geo {
// intersects(s,t,eps): 閉線分同士が端点・重なりを含め交わるか。O(1)。
template<class T, class C> bool intersects(const Segment<T, C>& s, const Segment<T, C>& t, Epsilon eps = {}) {
    int a = orientation(s.a, s.b, t.a, eps), b = orientation(s.a, s.b, t.b, eps);
    if constexpr (std::is_integral_v<T>) {
        if (a * b > 0) return false;
    }
    int c = orientation(t.a, t.b, s.a, eps), d = orientation(t.a, t.b, s.b, eps);
    if (a == 0 && on_segment(t.a, s, eps)) return true;
    if (b == 0 && on_segment(t.b, s, eps)) return true;
    if (c == 0 && on_segment(s.a, t, eps)) return true;
    if (d == 0 && on_segment(s.b, t, eps)) return true;
    return a * b < 0 && c * d < 0;
}
}
#endif
