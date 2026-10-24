// SPDX-License-Identifier: CC0-1.0
#include "../Math/Zeta/Set_Transform.cpp"
#include <cassert>
#include <cstdint>
#include <vector>

int main() {
    const std::vector<std::int64_t> original{1, 2, 3, 4};
    auto values = original;
    subset_zeta_transform(values);
    assert((values == std::vector<std::int64_t>{1, 3, 4, 10}));
    subset_mobius_transform(values);
    assert(values == original);
    superset_zeta_transform(values);
    assert((values == std::vector<std::int64_t>{10, 6, 7, 4}));
    superset_mobius_transform(values);
    assert(values == original);
}
