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
    geo::PointI a, b; std::cin >> a.x >> a.y >> b.x >> b.y;
    int q; std::cin >> q;
    while (q--) {
        geo::PointI p; std::cin >> p.x >> p.y;
        const int orientation = geo::orientation(a, b, p);
        if (orientation > 0) std::cout << "COUNTER_CLOCKWISE\n";
        else if (orientation < 0) std::cout << "CLOCKWISE\n";
        else if ((b - a).dot(p - a) < 0) std::cout << "ONLINE_BACK\n";
        else if (a.dist2(p) > a.dist2(b)) std::cout << "ONLINE_FRONT\n";
        else std::cout << "ON_SEGMENT\n";
    }
}
