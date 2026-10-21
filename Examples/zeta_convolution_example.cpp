#include "../Math/Zeta/Gcd_Lcm_Convolution.cpp"
#include <cassert>
#include <vector>

int main() {
    const std::vector<int> f{2, 3, 5}, g{7, 11, 13};
    IntegerSieve sieve(2);
    assert((gcd_convolution(f, g) == std::vector<int>{14, 170, 126}));
    assert((lcm_convolution(f, g) == std::vector<int>{118, 33, 159}));
    assert(gcd_convolution(f, g, sieve) == gcd_convolution(f, g));
    assert(lcm_convolution(f, g, sieve) == lcm_convolution(f, g));
}
