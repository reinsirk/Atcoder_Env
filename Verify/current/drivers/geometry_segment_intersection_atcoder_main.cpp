// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <cstdint>

int main() {
    geo::Segment<std::int64_t> a, b;
    verify_in >> a.a.x >> a.a.y >> a.b.x >> a.b.y;
    verify_in >> b.a.x >> b.a.y >> b.b.x >> b.b.y;
    verify_out << (geo::intersects(a, b) ? "Yes" : "No") << '\n';
}
