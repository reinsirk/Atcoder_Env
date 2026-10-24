#ifndef SIRKIT_GEO_SEGMENT_PREDICATES_DEFINED
#define SIRKIT_GEO_SEGMENT_PREDICATES_DEFINED
namespace geo {
// SIRKIT_API_BEGIN
// [geo::intersects]
// intersects(s,t,eps): 閉線分同士が端点・重なりを含め交わるか。O(1)。
// SIRKIT_API_END
template<class T, class C> bool intersects(const Segment<T, C>& s, const Segment<T, C>& t, Epsilon eps = {}) {
    if constexpr (std::is_integral_v<T>) {
        const int a = orientation(s.a, s.b, t.a, eps), b = orientation(s.a, s.b, t.b, eps);
        if (a * b > 0) return false;
        // 整数計算で共線と分かった場合は、変化する1軸だけを調べる。
        auto in_box = [](const Point<T, C>& p, const Segment<T, C>& line) {
            if constexpr (std::numeric_limits<C>::is_integer) {
                if (line.a.x != line.b.x)
                    return std::min(line.a.x, line.b.x) <= p.x && p.x <= std::max(line.a.x, line.b.x);
                if (line.a.y != line.b.y)
                    return std::min(line.a.y, line.b.y) <= p.y && p.y <= std::max(line.a.y, line.b.y);
                return p == line.a;
            }
            return std::min(line.a.x, line.b.x) <= p.x && p.x <= std::max(line.a.x, line.b.x) &&
                   std::min(line.a.y, line.b.y) <= p.y && p.y <= std::max(line.a.y, line.b.y);
        };
        if (a == 0) {
            if (b != 0) return in_box(t.a, s);
            if (s.a == s.b) return orientation(t.a, t.b, s.a, eps) == 0 && in_box(s.a, t);
            if constexpr (std::numeric_limits<C>::is_integer) {
                if (s.a.x != s.b.x)
                    return std::max(s.a.x, s.b.x) >= std::min(t.a.x, t.b.x) &&
                           std::max(t.a.x, t.b.x) >= std::min(s.a.x, s.b.x);
                return std::max(s.a.y, s.b.y) >= std::min(t.a.y, t.b.y) &&
                       std::max(t.a.y, t.b.y) >= std::min(s.a.y, s.b.y);
            }
            return std::max(s.a.x, s.b.x) >= std::min(t.a.x, t.b.x) &&
                   std::max(t.a.x, t.b.x) >= std::min(s.a.x, s.b.x) &&
                   std::max(s.a.y, s.b.y) >= std::min(t.a.y, t.b.y) &&
                   std::max(t.a.y, t.b.y) >= std::min(s.a.y, s.b.y);
        }
        if (b == 0) return in_box(t.b, s);
        const int c = orientation(t.a, t.b, s.a, eps), d = orientation(t.a, t.b, s.b, eps);
        return c * d <= 0;
    } else {
        const int a = orientation(s.a, s.b, t.a, eps), b = orientation(s.a, s.b, t.b, eps);
        const int c = orientation(t.a, t.b, s.a, eps), d = orientation(t.a, t.b, s.b, eps);
        if (a == 0 && on_segment(t.a, s, eps)) return true;
        if (b == 0 && on_segment(t.b, s, eps)) return true;
        if (c == 0 && on_segment(s.a, t, eps)) return true;
        if (d == 0 && on_segment(s.b, t, eps)) return true;
        return a * b < 0 && c * d < 0;
    }
}
}
#endif
