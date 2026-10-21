#include "../Math/Zeta/Divisor_Multiple_Transform.cpp"
#include <cassert>
#include <vector>

int main() {
    IntegerSieve sieve(6);
    const std::vector<int> original{7, 1, 2, 3, 4, 5, 6};
    auto values = original;
    multiple_zeta_transform(values, sieve);
    assert(values[0] == 7 && values[1] == 21 && values[2] == 12);
    multiple_mobius_transform(values, sieve);
    assert(values == original);
    divisor_zeta_transform(values, sieve);
    assert(values[6] == 12);
    divisor_mobius_transform(values, sieve);
    assert(values == original);
    multiple_zeta_transform(values);
    multiple_mobius_transform(values);
    divisor_zeta_transform(values);
    divisor_mobius_transform(values);
    assert(values == original);
}
