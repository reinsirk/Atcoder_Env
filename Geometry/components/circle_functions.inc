#ifndef SIRKIT_GEO_CIRCLE_DEFINED
#define SIRKIT_GEO_CIRCLE_DEFINED
namespace geo {
// 円周・円盤の関数。eps は絶対・相対許容幅、交点は RealPoint。
// circumcircle(a,b,c,eps): 3点を通る円。一直線・重複点なら nullopt。O(1)。
// validate(c): 中心・半径が有限かつ半径非負かを検査し、不正なら domain_error。省略設定でも有効。O(1)。
// contains(c,p,eps): 円盤の外・円周上・内を距離で判定する。O(1)。
// intersection(c,l,eps): 円周と非零長直線の交点を直線の向き順に0/1/2点返す。O(1)。
// intersection(c,s,eps): 円周と閉線分の交点を線分の向き順に返す。零長線分も可。O(1)。
// intersection(a,b,eps): 円周同士の交差種別と交点。半径0同士の同一点は Tangent。O(1)。

template<class T, class C> std::optional<Circle> circumcircle(const Point<T, C>& a, const Point<T, C>& b, const Point<T, C>& c, Epsilon eps = {}) {
    if (orientation(a, b, c, eps) == 0) return std::nullopt;
    C ux = C(b.x) - C(a.x), uy = C(b.y) - C(a.y), vx = C(c.x) - C(a.x), vy = C(c.y) - C(a.y);
    C u2 = ux * ux + uy * uy, v2 = vx * vx + vy * vy, determinant = ux * vy - uy * vx;
    long double den = 2 * static_cast<long double>(determinant);
    long double dx = (static_cast<long double>(u2) * static_cast<long double>(vy) - static_cast<long double>(v2) * static_cast<long double>(uy)) / den;
    long double dy = (static_cast<long double>(ux) * static_cast<long double>(v2) - static_cast<long double>(vx) * static_cast<long double>(u2)) / den;
    return Circle{{static_cast<long double>(a.x) + dx, static_cast<long double>(a.y) + dy}, std::hypot(dx, dy)};
}

inline void validate(const Circle& c) {
    if (!std::isfinite(c.center.x) || !std::isfinite(c.center.y) || !std::isfinite(c.radius) || c.radius < 0)
        throw std::domain_error("invalid circle");
}

inline Location contains(const Circle& c, const RealPoint& p, Epsilon eps = {}) {
#ifndef SKIP_RUNTIME_CHECKS
    validate(c);
    if (!std::isfinite(p.x) || !std::isfinite(p.y)) throw std::domain_error("nonfinite point");
#endif
    long double d = c.center.dist(p), tolerance = eps.tolerance(std::max(d, c.radius));
    if (d - c.radius > tolerance) return Location::Outside;
    if (c.radius - d > tolerance) return Location::Inside;
    return Location::Boundary;
}

inline std::vector<RealPoint> intersection(const Circle& c, const Line<long double>& l, Epsilon eps = {}) {
#ifndef SKIP_RUNTIME_CHECKS
    validate(c);
#endif
    RealPoint direction = l.b - l.a;
    if (direction.x == 0 && direction.y == 0) throw std::domain_error("degenerate line");
    RealPoint unit = direction.normalized(), from = c.center - l.a;
    long double along = from.dot(unit), signed_distance = from.cross(unit), d = std::abs(signed_distance);
    long double tolerance = eps.tolerance(std::max(c.radius, d));
    if (d - c.radius > tolerance) return {};
    RealPoint foot = l.a + unit * along;
    if (std::abs(d - c.radius) <= tolerance) return {foot};
    long double ratio = d / c.radius;
    long double delta = c.radius * std::sqrt(std::max(0.0L, (1 - ratio) * (1 + ratio)));
    return {foot - unit * delta, foot + unit * delta};
}

inline std::vector<RealPoint> intersection(const Circle& c, const Segment<long double>& s, Epsilon eps = {}) {
#ifndef SKIP_RUNTIME_CHECKS
    validate(c);
#endif
    if (s.a == s.b) return contains(c, s.a, eps) == Location::Boundary ? std::vector<RealPoint>{s.a} : std::vector<RealPoint>{};
    auto points = intersection(c, Line<long double>{s.a, s.b}, eps);
    std::size_t write = 0;
    for (const auto& p : points) if (on_segment(p, s, eps)) points[write++] = p;
    points.resize(write);
    return points;
}

inline CircleIntersection intersection(const Circle& a, const Circle& b, Epsilon eps = {}) {
#ifndef SKIP_RUNTIME_CHECKS
    validate(a); validate(b);
#endif
    RealPoint delta = b.center - a.center;
    long double d = delta.norm(), r = a.radius, s = b.radius;
    long double tolerance = eps.tolerance(std::max({d, r, s}));
    if (d == 0) {
        if (std::abs(r - s) > tolerance) return {};
        if (r == 0 || s == 0) return {CircleIntersectionKind::Tangent, {a.center}};
        return {CircleIntersectionKind::Coincident, {}};
    }
    if (r == 0) {
        if (std::abs(d - s) <= tolerance) return {CircleIntersectionKind::Tangent, {a.center}};
        return {};
    }
    if (s == 0) {
        if (std::abs(d - r) <= tolerance) return {CircleIntersectionKind::Tangent, {b.center}};
        return {};
    }
    long double gap_out = (d - r) - s;
    long double difference = r - s;
    long double gap_in = std::abs(difference) - d;
    if (gap_out > tolerance || gap_in > tolerance) return {};
    RealPoint unit = delta / d;
    // 許容幅内の非交差・厳密接触は、一方の円周上に接点を置く。
    if (gap_out >= 0) return {CircleIntersectionKind::Tangent, {a.center + unit * r}};
    if (gap_in >= 0) return {CircleIntersectionKind::Tangent, {a.center + unit * (difference >= 0 ? r : -r)}};
    long double along = d / 2 + (difference / d) * (r / 2 + s / 2);
    along = std::clamp(along, -r, r);
    RealPoint foot = a.center + unit * along;
    // ほぼ同心でも交点の高さが大きい場合は2点を保つ。
    if (std::abs(std::abs(along) - r) <= tolerance && std::abs(std::abs(d - along) - s) <= tolerance)
        return {CircleIntersectionKind::Tangent, {foot}};
    long double ratio = along / r;
    long double height = r * std::sqrt(std::max(0.0L, (1 - ratio) * (1 + ratio)));
    RealPoint offset{-unit.y * height, unit.x * height};
    return {CircleIntersectionKind::TwoPoints, {foot + offset, foot - offset}};
}
}
#endif
