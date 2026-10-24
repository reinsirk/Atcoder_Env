#ifndef SIRKIT_GEO_POLYGON_DEFINED
#define SIRKIT_GEO_POLYGON_DEFINED
namespace geo {
// SIRKIT_API_BEGIN
// [多角形・三角形]
// 多角形は vector<Point<T,C>>。先頭点の重複による閉包は不要。
// signed_area2(poly): 符号付き面積の2倍を C で返す。反時計回りが正。O(N)、空は0。
// area(poly): 面積の絶対値を long double で返す。O(N)。
// signed_triangle_area2(a,b,c): 三角形の符号付き面積の2倍をCで返す。差・積・減算がCに収まる入力。O(1)。
// triangle_area(a,b,c): 三角形の面積をlong doubleで返す。退化三角形は0。配列確保なし。O(1)。
// perimeter(poly): 閉じた辺の総延長。0/1点は0、2点は往復分。O(N)。
// contains(poly,p,eps): 単純多角形の内外・境界を巻き数で返す。0点は外、1/2点は境界のみ。O(N)。
// convex_hull(points,eps): 完全一致を除き、辞書順最小点から反時計回りの凸包。一直線は両端のみ。O(N log N)。
// convex_hull_sorted(points,eps): 辞書順整列済みのgeo::Pointから反時計回りの凸包を返す。重複可。O(N)。
// centroid(poly,eps): 単純多角形の面積重心。3点未満または面積0なら nullopt。O(N)。
// convex_cut(poly,l,eps): 凸多角形と有向非零長直線の左側（境界含む）との共通部分。O(N)。
// SIRKIT_API_END

template<class T, class C> C signed_area2(const std::vector<Point<T, C>>& poly) {
    if (poly.size() < 3) return C{0};
    const C origin_x = C(poly[0].x), origin_y = C(poly[0].y);
    // 直前の頂点の座標差を次の三角形にも使う。
    C previous_x = C(poly[1].x) - origin_x, previous_y = C(poly[1].y) - origin_y;
    C result = 0;
    const auto* it = poly.data() + 2;
    const auto* end = poly.data() + poly.size();
    for (; it != end; ++it) {
        const C x = C(it->x) - origin_x, y = C(it->y) - origin_y;
        result += previous_x * y - previous_y * x;
        previous_x = x; previous_y = y;
    }
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
        if constexpr (std::is_integral_v<T>) {
            if ((a.y < p.y && b.y < p.y) || (a.y > p.y && b.y > p.y)) continue;
        }
        int side = orientation(a, b, p, eps);
        if (side == 0) {
            if constexpr (std::is_integral_v<T>) {
                if (std::min(a.x, b.x) <= p.x && p.x <= std::max(a.x, b.x) &&
                    std::min(a.y, b.y) <= p.y && p.y <= std::max(a.y, b.y)) return Location::Boundary;
            } else if (on_segment(p, Segment<T, C>{a, b}, eps)) return Location::Boundary;
        }
        if (a.y <= p.y && p.y < b.y && side > 0) ++winding;
        if (b.y <= p.y && p.y < a.y && side < 0) --winding;
    }
    return winding != 0 ? Location::Inside : Location::Outside;
}

}
#ifndef SIRKIT_GEO_CONVEX_HULL_IMPL_DEFINED
#define SIRKIT_GEO_CONVEX_HULL_IMPL_DEFINED
namespace reinsirk::detail {
template<bool Sorted, class T, class C>
std::vector<geo::Point<T, C>> convex_hull_impl(std::vector<geo::Point<T, C>> points, geo::Epsilon eps) {
    constexpr bool narrow_integer = std::is_integral_v<T> && sizeof(T) <= 8 &&
        std::numeric_limits<C>::is_integer && std::numeric_limits<C>::digits >= 63;
    bool narrow = false;
    bool radix_unique = false;
    if constexpr (narrow_integer) {
        if (points.size() <= 1) return points;
        T min_x = points.front().x, max_x = min_x;
        T min_y = points.front().y, max_y = min_y;
        [[maybe_unused]] bool ordered = true;
        for (std::size_t i = 1; i < points.size(); ++i) {
            const auto& p = points[i];
            min_x = std::min(min_x, p.x); max_x = std::max(max_x, p.x);
            min_y = std::min(min_y, p.y); max_y = std::max(max_y, p.y);
#ifndef SKIP_RUNTIME_CHECKS
            if constexpr (Sorted) {
                if (p < points[i - 1]) throw std::invalid_argument("convex_hull_sorted: points are not sorted");
            }
#endif
            if constexpr (!Sorted) ordered = ordered && !(p < points[i - 1]);
        }
        const auto span_x = std::uint64_t(max_x) - std::uint64_t(min_x);
        const auto span_y = std::uint64_t(max_y) - std::uint64_t(min_y);
        constexpr auto maximum = std::uint64_t(std::numeric_limits<std::int64_t>::max());
        // 積を別々に比較するため、外積の減算は行わない。
        narrow = span_x <= maximum && span_y <= maximum && (span_y == 0 || span_x <= maximum / span_y);
        if constexpr (!Sorted) {
            if (!ordered) {
                if (points.size() >= 2048 && span_x <= std::numeric_limits<std::uint32_t>::max() && span_y <= std::numeric_limits<std::uint32_t>::max()) {
                    // 領域を先に確保し、キー生成中の容量判定を省く。
                    std::vector<std::uint64_t> keys(points.size());
                    std::uint64_t varying = 0;
                    const auto first = ((std::uint64_t(points.front().x) - std::uint64_t(min_x)) << 32) |
                                       (std::uint64_t(points.front().y) - std::uint64_t(min_y));
                    for (std::size_t i = 0; i < points.size(); ++i) {
                        const auto key = ((std::uint64_t(points[i].x) - std::uint64_t(min_x)) << 32) |
                                         (std::uint64_t(points[i].y) - std::uint64_t(min_y));
                        keys[i] = key;
                        varying |= key ^ first;
                    }
                    std::vector<std::uint64_t> temp(keys.size());
                    // 多くの桁が変化する大入力では、全桁の度数をまとめて数える。
                    std::array<std::array<std::uint32_t, 256>, 8> counts{};
                    unsigned active = 0;
                    for (unsigned k = 0; k < 8; ++k) active += ((varying >> (k * 8)) & 255) != 0;
                    const bool combined = keys.size() >= 16384 && keys.size() <= std::numeric_limits<std::uint32_t>::max() && active >= 6;
                    if (combined) {
                        for (auto key : keys) {
                            ++counts[0][(key >> 0) & 255];
                            ++counts[1][(key >> 8) & 255];
                            ++counts[2][(key >> 16) & 255];
                            ++counts[3][(key >> 24) & 255];
                            ++counts[4][(key >> 32) & 255];
                            ++counts[5][(key >> 40) & 255];
                            ++counts[6][(key >> 48) & 255];
                            ++counts[7][(key >> 56) & 255];
                        }
                    }
                    for (unsigned shift = 0; shift < 64; shift += 8) {
                        if (((varying >> shift) & 255) == 0) continue;
                        std::array<std::size_t, 256> count{};
                        if (combined) std::copy(counts[shift / 8].begin(), counts[shift / 8].end(), count.begin());
                        else for (auto key : keys) ++count[(key >> shift) & 255];
                        std::size_t sum = 0;
                        for (auto& n : count) {
                            const auto next = sum + n;
                            n = sum;
                            sum = next;
                        }
                        for (auto key : keys) temp[count[(key >> shift) & 255]++] = key;
                        keys.swap(temp);
                    }
                    // 座標へ戻す前に、同じ点を表すキーを除く。
                    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
                    points.resize(keys.size());
                    radix_unique = true;
                    for (std::size_t i = 0; i < keys.size(); ++i) {
                        points[i].x = static_cast<T>(std::int64_t(min_x) + std::int64_t(keys[i] >> 32));
                        points[i].y = static_cast<T>(std::int64_t(min_y) + std::int64_t(std::uint32_t(keys[i])));
                    }
                } else std::sort(points.begin(), points.end());
            }
        }
    } else if constexpr (!Sorted) std::sort(points.begin(), points.end());
    if (!radix_unique) points.erase(std::unique(points.begin(), points.end(), [](const auto& a, const auto& b) {
#ifndef SKIP_RUNTIME_CHECKS
        if constexpr (Sorted && !narrow_integer) {
            if (b < a) throw std::invalid_argument("convex_hull_sorted: points are not sorted");
        }
#endif
        return a == b;
    }), points.end());
    if (points.size() <= 1) {
        return points;
    }
    std::vector<geo::Point<T, C>> hull;
    auto left_turn = [&](const geo::Point<T, C>& a, const geo::Point<T, C>& b, const geo::Point<T, C>& c) {
        if constexpr (std::is_integral_v<T>) {
            if (narrow) {
                const auto ux = std::int64_t(b.x) - std::int64_t(a.x), uy = std::int64_t(b.y) - std::int64_t(a.y);
                const auto vx = std::int64_t(c.x) - std::int64_t(a.x), vy = std::int64_t(c.y) - std::int64_t(a.y);
                return ux * vy > uy * vx;
            }
            return a.orient(b, c) > 0;
        }
        else return geo::orientation(a, b, c, eps) > 0;
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
}
#endif
namespace geo {
template<class T, class C> std::vector<Point<T, C>> convex_hull(std::vector<Point<T, C>> points, Epsilon eps = {}) {
    return reinsirk::detail::convex_hull_impl<false>(std::move(points), eps);
}

}
#ifndef SIRKIT_CONVEX_HULL_SORTED_DEFINED
#define SIRKIT_CONVEX_HULL_SORTED_DEFINED
template<class T, class C>
std::vector<geo::Point<T, C>> convex_hull_sorted(std::vector<geo::Point<T, C>> points, geo::Epsilon eps = {}) {
    return reinsirk::detail::convex_hull_impl<true>(std::move(points), eps);
}
#endif
namespace geo {

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
#ifndef SIRKIT_TRIANGLE_AREA_DEFINED
#define SIRKIT_TRIANGLE_AREA_DEFINED
template<class T, class C>
constexpr C signed_triangle_area2(const geo::Point<T, C>& a, const geo::Point<T, C>& b, const geo::Point<T, C>& c) {
    return a.orient(b, c);
}
template<class T, class C>
long double triangle_area(const geo::Point<T, C>& a, const geo::Point<T, C>& b, const geo::Point<T, C>& c) {
    return std::abs(static_cast<long double>(signed_triangle_area2(a, b, c))) / 2;
}
#endif
namespace geo {
}
#endif
