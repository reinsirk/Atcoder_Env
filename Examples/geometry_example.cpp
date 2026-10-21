// SPDX-License-Identifier: CC0-1.0
#include "../Geometry/geometory.cpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>

int main() {
    using geo::PointI;
    const PointI a{0, 0}, b{4, 0}, c{0, 3};
    assert(a.dist(b) == 4);
    assert(geo::orientation(a, b, c) == 1);
    const geo::Segment<std::int64_t> s{a, b}, t{{2, -1}, {2, 1}};
    assert(geo::intersects(s, t));
    const auto hit = geo::intersection(s, t);
    assert(hit.kind == geo::SegmentIntersectionKind::Point);
    assert(std::abs(hit.points[0].x - 2) < 1e-12L);
    const std::vector<PointI> points{a, b, c, {1, 1}};
    const auto hull = geo::convex_hull(points);
    assert(hull.size() == 3 && geo::area(hull) == 6);
    assert(geo::contains(hull, PointI{1, 1}) == geo::Location::Inside);
    const auto cut = geo::convex_cut(hull, geo::Line<std::int64_t>{{2, -1}, {2, 4}});
    assert(std::abs(geo::area(cut) - 4.5L) < 1e-12L);
    const auto circle = geo::circumcircle(a, b, c);
    assert(circle && std::abs(circle->radius - 2.5L) < 1e-12L);
    const auto circle_hit = geo::intersection(*circle, geo::Line<long double>{geo::to_real(a), geo::to_real(b)});
    assert(circle_hit.size() == 2);
}
