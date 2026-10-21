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
    int q; std::cin >> q;
    std::cout << std::fixed << std::setprecision(12);
    while (q--) {
        geo::Segment<long double> a, b;
        std::cin >> a.a.x >> a.a.y >> a.b.x >> a.b.y >> b.a.x >> b.a.y >> b.b.x >> b.b.y;
        std::cout << geo::distance(a, b) << '\n';
    }
}
