// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

int main() {
    const geo::PointI a{0, 0}, b{4, 0}, c{0, 3};
    assert(a.dist2(b) == 16 && a.orient(b, c) == 12);
    assert((b.rotated90() == geo::PointI{0, 4}));
    assert(geo::orientation(a, b, c) == 1);
    const geo::PointLD p{3, 4};
    assert(std::abs(p.normalized().norm() - 1) < 1e-12L);
#ifdef REINSIRK_GEO_LINEAR_DEFINED
    const geo::Segment<std::int64_t> first{{0, 0}, {4, 0}}, overlap{{2, 0}, {6, 0}}, singleton{{1, 0}, {1, 0}};
    assert(geo::intersection(first, overlap).kind == geo::SegmentIntersectionKind::Overlap);
    assert(geo::intersection(first, singleton).kind == geo::SegmentIntersectionKind::Point);
    assert(geo::intersection(geo::Line<std::int64_t>{{0, 0}, {1, 0}}, geo::Line<std::int64_t>{{0, 1}, {1, 1}}).kind == geo::LineIntersectionKind::None);
    assert(std::abs(geo::distance(geo::PointI{1, 2}, singleton) - 2) < 1e-12L);
#endif
#ifdef REINSIRK_GEO_POLYGON_DEFINED
    const auto hull = geo::convex_hull(std::vector<geo::PointI>{a, b, c, {1, 1}, a});
    assert(hull.size() == 3 && geo::area(hull) == 6);
    assert(geo::contains(hull, geo::PointI{1, 1}) == geo::Location::Inside);
    assert(geo::contains(hull, a) == geo::Location::Boundary);
    auto center = geo::centroid(hull);
    assert(center && std::abs(center->x - 4.0L / 3) < 1e-12L && std::abs(center->y - 1) < 1e-12L);
    assert(!geo::centroid(std::vector<geo::PointI>{a, b}));
    assert(geo::convex_hull(std::vector<geo::PointI>{{0, 0}, {1, 0}, {2, 0}}).size() == 2);
#endif
#ifdef REINSIRK_GEO_CIRCLE_DEFINED
    const auto circle = geo::circumcircle(a, b, c);
    assert(circle && std::abs(circle->radius - 2.5L) < 1e-12L);
    assert(!geo::circumcircle(a, a, b));
    const geo::Circle unit{{0, 0}, 1};
    assert(geo::intersection(unit, geo::Circle{{3, 0}, 1}).kind == geo::CircleIntersectionKind::None);
    assert(geo::intersection(unit, unit).kind == geo::CircleIntersectionKind::Coincident);
    assert(geo::intersection(unit, geo::Circle{{2, 0}, 1}).kind == geo::CircleIntersectionKind::Tangent);
    assert(geo::intersection(unit, geo::Segment<long double>{{-2, 0}, {2, 0}}).size() == 2);
    assert(geo::intersection(unit, geo::Segment<long double>{{2, 0}, {3, 0}}).empty());
#endif
    std::cout << "OK\n";
}
