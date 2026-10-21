// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <array>

int main() {
    std::array<geo::PointI, 4> points;
    for (auto& p : points) verify_in >> p.x >> p.y;
    bool convex = true;
    for (int i = 0; i < 4; ++i)
        if (geo::orientation(points[i], points[(i + 1) % 4], points[(i + 2) % 4]) <= 0)
            convex = false;
    verify_out << (convex ? "Yes" : "No") << '\n';
}
