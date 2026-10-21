// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <algorithm>

int main() {
    geo::Circle a, b;
    verify_in >> a.center.x >> a.center.y >> a.radius;
    verify_in >> b.center.x >> b.center.y >> b.radius;
    const auto result = geo::intersection(a, b);
    const auto distance2 = a.center.dist2(b.center);
    const auto radius = std::max(a.radius, b.radius);
    int answer;
    if (result.kind == geo::CircleIntersectionKind::Coincident) answer = 2;
    else if (result.kind == geo::CircleIntersectionKind::TwoPoints) answer = 3;
    else if (result.kind == geo::CircleIntersectionKind::Tangent)
        answer = distance2 < radius * radius ? 2 : 4;
    else answer = distance2 < radius * radius ? 1 : 5;
    verify_out << answer << '\n';
}
