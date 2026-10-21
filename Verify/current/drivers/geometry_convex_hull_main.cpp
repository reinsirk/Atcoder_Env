// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <utility>
#include <vector>

int main() {
    int t{};
    verify_in >> t;
    while (t--) {
        int n{};
        verify_in >> n;
        std::vector<geo::PointI> points(n);
        for (auto& p : points) verify_in >> p.x >> p.y;
        const auto hull = geo::convex_hull(std::move(points));
        verify_out << hull.size() << '\n';
        for (const auto& p : hull) verify_out << p.x << ' ' << p.y << '\n';
    }
}
