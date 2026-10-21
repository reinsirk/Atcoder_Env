#include "../Math/Zeta/Sparse_Transform.cpp"
#include <cassert>
#include <utility>
#include <vector>

int main() {
    IntegerSieve sieve(100);
    const std::vector<std::pair<int, int>> original{{1, 1}, {2, 2}, {3, 3}, {6, 6}};
    auto values = original;
    sparse_multiple_zeta_transform(values, sieve);
    assert(values.front().second == 12);
    sparse_multiple_mobius_transform(values, sieve);
    assert(values == original);
}
