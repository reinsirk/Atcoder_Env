#include "../Math/Zeta/Bitwise_Convolution.cpp"
#include <cassert>
#include <vector>

int main() {
    const std::vector<int> f{1, 2}, g{3, 4};
    assert((bitwise_and_convolution(f, g) == std::vector<int>{13, 8}));
    assert((bitwise_or_convolution(f, g) == std::vector<int>{3, 18}));
}
