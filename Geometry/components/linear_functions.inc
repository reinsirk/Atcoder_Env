#ifndef SIRKIT_GEO_LINEAR_DEFINED
#define SIRKIT_GEO_LINEAR_DEFINED
namespace geo {
// SIRKIT_API_BEGIN
// [geo::直線・線分の関数]
// 点・直線・閉線分の関数。交点・射影・距離は long double で近似する。
// projection(p,l): long double 変換後も非零長の直線 l 上への射影。O(1)。
// reflection(p,l): 直線 l に関して対称な点。projection と同じ前提。O(1)。
// closest_point(p,s): 閉線分上の最近点。long double 変換後の零長線分は a を返す。O(1)。
// distance(p,l): 点と非零長直線の距離。O(1)。
// distance(p,s): 点と閉線分の距離（零長可）。O(1)。
// distance(s,t,eps): 閉線分間距離。intersects が真なら0。O(1)。
// intersection(l,m,eps): 非零長直線の交差種別と近似交点。O(1)。
// intersection(s,t,eps): 閉線分の交点、または元の座標で辞書順に選んだ重なりの両端点。O(1)。
// SIRKIT_API_END

template<class T, class C> RealPoint projection(const Point<T, C>& p, const Line<T, C>& l) {
    RealPoint a = to_real(l.a), b = to_real(l.b), q = to_real(p);
    RealPoint d = b - a;
    if (d.x == 0 && d.y == 0) throw std::domain_error("degenerate line");
    RealPoint u = d.normalized();
    return a + u * ((q - a).dot(u));
}

template<class T, class C> RealPoint reflection(const Point<T, C>& p, const Line<T, C>& l) {
    RealPoint q = projection(p, l);
    return q + (q - to_real(p));
}

template<class T, class C> RealPoint closest_point(const Point<T, C>& p, const Segment<T, C>& s) {
    RealPoint a = to_real(s.a), b = to_real(s.b), q = to_real(p), d = b - a;
    if (d.x == 0 && d.y == 0) return a;
    RealPoint u = d.normalized();
    long double along = (q - a).dot(u);
    if (along <= 0) return a;
    if ((q - b).dot(u) >= 0) return b;
    return a + u * along;
}

template<class T, class C> long double distance(const Point<T, C>& p, const Line<T, C>& l) {
    RealPoint a = to_real(l.a), b = to_real(l.b), q = to_real(p), d = b - a;
    if (d.x == 0 && d.y == 0) throw std::domain_error("degenerate line");
    return std::abs((q - a).cross(d.normalized()));
}

template<class T, class C> long double distance(const Point<T, C>& p, const Segment<T, C>& s) {
    RealPoint a = to_real(s.a), b = to_real(s.b), q = to_real(p), d = b - a;
    if (d.x == 0 && d.y == 0) return q.dist(a);
    // 整数の差の二乗和が表せる場合は、方向長を直接計算する。
    RealPoint u;
    if constexpr (std::is_integral_v<T> && sizeof(T) <= 8 &&
                  std::numeric_limits<long double>::max_exponent > 2 * (std::numeric_limits<T>::digits + 1) + 1) {
        const long double length = std::sqrt(d.x * d.x + d.y * d.y);
        u = {d.x / length, d.y / length};
    } else u = d.normalized();
    const RealPoint offset = q - a;
    if (offset.dot(u) <= 0) return q.dist(a);
    if ((q - b).dot(u) >= 0) return q.dist(b);
    return std::abs(offset.cross(u));
}

template<class T, class C> long double distance(const Segment<T, C>& s, const Segment<T, C>& t, Epsilon eps = {}) {
    if (intersects(s, t, eps)) return 0;
    return std::min({distance(s.a, t), distance(s.b, t), distance(t.a, s), distance(t.b, s)});
}

template<class T, class C> LineIntersection intersection(const Line<T, C>& l, const Line<T, C>& m, Epsilon eps = {}) {
    if (l.a == l.b || m.a == m.b) throw std::domain_error("degenerate line");
    C ux = C(l.b.x) - C(l.a.x), uy = C(l.b.y) - C(l.a.y);
    C vx = C(m.b.x) - C(m.a.x), vy = C(m.b.y) - C(m.a.y);
    C p = ux * vy, q = uy * vx, determinant = p - q;
    int sign;
    if constexpr (std::is_integral_v<T>) sign = (determinant > 0) - (determinant < 0);
    else sign = predicate_sign<T, C>(determinant, std::abs(p) + std::abs(q), eps);
    if (sign == 0) {
        if (orientation(l.a, l.b, m.a, eps) == 0) return {LineIntersectionKind::Coincident, std::nullopt};
        return {};
    }
    C wx = C(m.a.x) - C(l.a.x), wy = C(m.a.y) - C(l.a.y);
    long double t = static_cast<long double>(wx * vy - wy * vx) / static_cast<long double>(determinant);
    return {LineIntersectionKind::Point, RealPoint{static_cast<long double>(l.a.x) + t * static_cast<long double>(ux), static_cast<long double>(l.a.y) + t * static_cast<long double>(uy)}};
}

template<class T, class C> SegmentIntersection intersection(const Segment<T, C>& s, const Segment<T, C>& t, Epsilon eps = {}) {
    std::array<Point<T, C>, 4> candidates;
    std::size_t n = 0;
    if (on_segment(s.a, t, eps)) candidates[n++] = s.a;
    if (on_segment(s.b, t, eps)) candidates[n++] = s.b;
    if (on_segment(t.a, s, eps)) candidates[n++] = t.a;
    if (on_segment(t.b, s, eps)) candidates[n++] = t.b;
    if (n != 0) {
        auto ends = std::minmax_element(candidates.begin(), candidates.begin() + static_cast<std::ptrdiff_t>(n));
        if (*ends.first == *ends.second) return {SegmentIntersectionKind::Point, {to_real(*ends.first)}};
        return {SegmentIntersectionKind::Overlap, {to_real(*ends.first), to_real(*ends.second)}};
    }
    int a = orientation(s.a, s.b, t.a, eps), b = orientation(s.a, s.b, t.b, eps);
    int c = orientation(t.a, t.b, s.a, eps), d = orientation(t.a, t.b, s.b, eps);
    if (!(a * b < 0 && c * d < 0)) return {};
    C ux = C(s.b.x) - C(s.a.x), uy = C(s.b.y) - C(s.a.y);
    C vx = C(t.b.x) - C(t.a.x), vy = C(t.b.y) - C(t.a.y);
    C wx = C(t.a.x) - C(s.a.x), wy = C(t.a.y) - C(s.a.y);
    C determinant = ux * vy - uy * vx;
    if (determinant == 0) return {};
    long double ratio = static_cast<long double>(wx * vy - wy * vx) / static_cast<long double>(determinant);
    return {SegmentIntersectionKind::Point, {{static_cast<long double>(s.a.x) + ratio * static_cast<long double>(ux), static_cast<long double>(s.a.y) + ratio * static_cast<long double>(uy)}}};
}
}
#endif
