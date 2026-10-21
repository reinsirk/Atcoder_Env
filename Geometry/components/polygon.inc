#ifndef REINSIRK_GEO_POLYGON_DEFINED
#define REINSIRK_GEO_POLYGON_DEFINED
namespace geo {
// 多角形は vector<Point<T,C>>。先頭点の重複による閉包は不要。
// signed_area2(poly): 符号付き面積の2倍を C で返す。反時計回りが正。O(N)、空は0。
// area(poly): 面積の絶対値を long double で返す。O(N)。
// perimeter(poly): 閉じた辺の総延長。0/1点は0、2点は往復分。O(N)。
// contains(poly,p,eps): 単純多角形の内外・境界を巻き数で返す。0点は外、1/2点は境界のみ。O(N)。
// convex_hull(points,eps): 完全一致を除き、辞書順最小点から反時計回りの凸包。一直線は両端のみ。O(N log N)。
// centroid(poly,eps): 単純多角形の面積重心。3点未満または面積0なら nullopt。O(N)。
// convex_cut(poly,l,eps): 凸多角形と有向非零長直線の左側（境界含む）との共通部分。O(N)。

template<class T, class C> C signed_area2(const std::vector<Point<T, C>>& poly) {
    C result = 0;
    for (std::size_t i = 2; i < poly.size(); ++i) result += poly[0].orient(poly[i - 1], poly[i]);
    return result;
}

template<class T, class C> long double area(const std::vector<Point<T, C>>& poly) {
    return std::abs(static_cast<long double>(signed_area2(poly))) / 2;
}

template<class T, class C> long double perimeter(const std::vector<Point<T, C>>& poly) {
    if (poly.size() < 2) return 0;
    long double result = 0;
    for (std::size_t i = 0; i < poly.size(); ++i) result += poly[i].dist(poly[i + 1 == poly.size() ? 0 : i + 1]);
    return result;
}

template<class T, class C> Location contains(const std::vector<Point<T, C>>& poly, const Point<T, C>& p, Epsilon eps = {}) {
    if (poly.empty()) return Location::Outside;
    if (poly.size() == 1) return on_segment(p, Segment<T, C>{poly[0], poly[0]}, eps) ? Location::Boundary : Location::Outside;
    std::int64_t winding = 0;
    for (std::size_t i = 0; i < poly.size(); ++i) {
        const auto& a = poly[i];
        const auto& b = poly[i + 1 == poly.size() ? 0 : i + 1];
        int side = orientation(a, b, p, eps);
        if (side == 0 && on_segment(p, Segment<T, C>{a, b}, eps)) return Location::Boundary;
        if (a.y <= p.y && p.y < b.y && side > 0) ++winding;
        if (b.y <= p.y && p.y < a.y && side < 0) --winding;
    }
    return winding != 0 ? Location::Inside : Location::Outside;
}

template<class T, class C> std::vector<Point<T, C>> convex_hull(std::vector<Point<T, C>> points, Epsilon eps = {}) {
    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end()), points.end());
    if (points.size() <= 1) {
        return points;
    }
    std::vector<Point<T, C>> hull;
    auto left_turn = [&](const Point<T, C>& a, const Point<T, C>& b, const Point<T, C>& c) {
        if constexpr (std::is_integral_v<T>) return a.orient(b, c) > 0;
        else return orientation(a, b, c, eps) > 0;
    };

    if (points.size() == hull.max_size()) throw std::length_error("geometry hull capacity");
    hull.reserve(points.size() + 1);
    for (const auto& p : points) {
        while (hull.size() >= 2 && !left_turn(hull[hull.size() - 2], hull.back(), p)) hull.pop_back();
        hull.push_back(p);
    }
    std::size_t lower = hull.size();
    for (std::size_t i = points.size() - 1; i-- > 0;) {
        while (hull.size() > lower && !left_turn(hull[hull.size() - 2], hull.back(), points[i])) hull.pop_back();
        hull.push_back(points[i]);
    }
    hull.pop_back();
    return hull;
}

template<class T, class C> std::optional<RealPoint> centroid(const std::vector<Point<T, C>>& poly, Epsilon eps = {}) {
    if (poly.size() < 3) return std::nullopt;
    C total = 0;
    long double sx = 0, sy = 0;
    [[maybe_unused]] C scale = 0;
    for (std::size_t i = 2; i < poly.size(); ++i) {
        C weight = poly[0].orient(poly[i - 1], poly[i]);
        total += weight;
        if constexpr (!std::is_integral_v<T>) scale += std::abs(weight);
        long double ux = static_cast<long double>(C(poly[i - 1].x) - C(poly[0].x)), uy = static_cast<long double>(C(poly[i - 1].y) - C(poly[0].y));
        long double vx = static_cast<long double>(C(poly[i].x) - C(poly[0].x)), vy = static_cast<long double>(C(poly[i].y) - C(poly[0].y));
        sx += (ux + vx) * static_cast<long double>(weight);
        sy += (uy + vy) * static_cast<long double>(weight);
    }
    if (predicate_sign<T, C>(total, scale, eps) == 0) return std::nullopt;
    long double denominator = 3 * static_cast<long double>(total);
    return RealPoint{static_cast<long double>(poly[0].x) + sx / denominator, static_cast<long double>(poly[0].y) + sy / denominator};
}

template<class T, class C> std::vector<RealPoint> convex_cut(const std::vector<Point<T, C>>& poly, const Line<T, C>& line, Epsilon eps = {}) {
    if (line.a == line.b) throw std::domain_error("degenerate line");
    std::vector<RealPoint> result;
    result.reserve(poly.size());
    if (poly.empty()) return result;
    for (std::size_t i = 0; i < poly.size(); ++i) {
        const auto& a = poly[i];
        const auto& b = poly[i + 1 == poly.size() ? 0 : i + 1];
        int sa = orientation(line.a, line.b, a, eps), sb = orientation(line.a, line.b, b, eps);
        auto append = [&](const RealPoint& p) { if (result.empty() || result.back() != p) result.push_back(p); };
        if (sa >= 0) append(to_real(a));
        if (sa * sb < 0) {
            C da = line.a.orient(line.b, a), db = line.a.orient(line.b, b);
            long double u = static_cast<long double>(da) / (static_cast<long double>(da) - static_cast<long double>(db));
            append({static_cast<long double>(a.x) + u * static_cast<long double>(C(b.x) - C(a.x)), static_cast<long double>(a.y) + u * static_cast<long double>(C(b.y) - C(a.y))});
        }
    }
    if (result.size() > 1 && result.front() == result.back()) result.pop_back();
    return result;
}
}
#endif
