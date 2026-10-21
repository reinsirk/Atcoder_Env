// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <vector>

int main() {
    std::vector<geo::PointI> triangle(3);
    for (auto& p : triangle) verify_in >> p.x >> p.y;
    verify_out.fixed(12);
    verify_out << geo::area(triangle) << '\n';
}
